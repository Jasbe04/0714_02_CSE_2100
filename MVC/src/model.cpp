/**
 * @file model.cpp
 * @brief MVC Model implementation.
 *
 * Contains ALL business logic: filesystem enumeration, navigation history,
 * clipboard management, file operations (via the FileOperation hierarchy),
 * and recursive search.
 *
 * There is NO GTK widget code in this file.  The only UI-adjacent dependency
 * is IProgressReporter (injected at construction), which is an abstract
 * interface — the Model does not know it talks to a GTK progress bar.
 */

#include "model.hpp"
#include "file_op.hpp"

#include <windows.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <glib.h> // g_utf8_strdown, g_strstr_len
#include <thread>
#include <atomic>
#include <functional>

#include <algorithm>

// ── Construction ──────────────────────────────────────────────────────────────

FileSystemModel::FileSystemModel(IProgressReporter *reporter)
    : m_reporter(reporter)
{
}

void FileSystemModel::setObserver(IModelObserver *observer)
{
    m_observer = observer;
}

// ── Navigation ────────────────────────────────────────────────────────────────

void FileSystemModel::loadDrives()
{
    m_depth = 0;
    // Clear stacks when returning to root drive list.
    while (!m_backStack.empty())
        m_backStack.pop();
    while (!m_forwardStack.empty())
        m_forwardStack.pop();
    m_currentPath.clear();

    if (m_observer)
        m_observer->onDrivesChanged(listDrives());
}

void FileSystemModel::openPath(const std::string &path, bool recordHistory)
{
    if (recordHistory && !m_currentPath.empty())
        m_backStack.push(m_currentPath);

    // Clear forward history whenever we navigate to a new place.
    while (!m_forwardStack.empty())
        m_forwardStack.pop();

    chdir(path.c_str());
    m_currentPath = path;
    ++m_depth;

    notifyDirectory();
}

void FileSystemModel::goToParent()
{
    if (m_depth == 0)
        return;

    // Save current position for forward navigation.
    m_forwardStack.push(m_currentPath);

    --m_depth;

    if (m_depth == 0)
    {
        m_currentPath.clear();
        if (m_observer)
            m_observer->onDrivesChanged(listDrives());
    }
    else
    {
        std::string prev = m_backStack.top();
        m_backStack.pop();
        chdir(prev.c_str());
        m_currentPath = prev;
        notifyDirectory();
    }
}

void FileSystemModel::goForward()
{
    if (m_forwardStack.empty())
        return;

    m_backStack.push(m_currentPath);

    std::string next = m_forwardStack.top();
    m_forwardStack.pop();

    chdir(next.c_str());
    m_currentPath = next;
    ++m_depth;

    notifyDirectory();
}

// ── File operations ───────────────────────────────────────────────────────────

bool FileSystemModel::createFolder(const std::string &name)
{
    bool created = false;

    if (name.empty())
    {
        if (mkdir("New Folder") == 0)
        {
            created = true;
        }
        else
        {
            for (int i = 1; i < 1000 && !created; ++i)
            {
                gchar *candidate = g_strdup_printf("New Folder (%d)", i);
                if (mkdir(candidate) == 0)
                    created = true;
                g_free(candidate);
            }
        }
    }
    else
    {
        created = (mkdir(name.c_str()) == 0);
    }

    if (created)
        notifyDirectory();

    return created;
}

void FileSystemModel::markCopy(const std::string &dir, const std::string &name)
{
    m_clipboard.sourceDir = dir;
    m_clipboard.sourceName = name;
    m_clipboard.isCopy = true;
    m_clipboard.isCut = false;

    if (m_observer)
        m_observer->onClipboardChanged(m_clipboard);
}

void FileSystemModel::markCut(const std::string &dir, const std::string &name)
{
    m_clipboard.sourceDir = dir;
    m_clipboard.sourceName = name;
    m_clipboard.isCut = true;
    m_clipboard.isCopy = false;

    if (m_observer)
        m_observer->onClipboardChanged(m_clipboard);
}

void FileSystemModel::paste()
{
    if (!m_clipboard.hasPending())
        return;

    // Capture clipboard state (may change during calculation)
    ClipboardState localClip = m_clipboard;

    // Get destination directory (current working directory)
    char cwd[MAX_PATH + 1];
    getcwd(cwd, sizeof(cwd));
    std::string destDir = cwd;
    std::string srcFull = localClip.sourceDir + "\\" + localClip.sourceName;
    std::string destFull = destDir + "\\" + localClip.sourceName;

    // Show "Calculating size..." dialog immediately
    m_reporter->showCalculating();

    // Launch background thread to compute total size
    std::thread([this, srcFull, destFull, localClip]()
                {
        long long totalSize = FileOperation::calculateTotalSize(srcFull);

        // Return to main thread to perform actual copy/move
        g_idle_add([](gpointer data) -> int {
            auto *params = static_cast<std::tuple<FileSystemModel*, std::string, std::string, ClipboardState, long long>*>(data);
            FileSystemModel *model = std::get<0>(*params);
            std::string src = std::get<1>(*params);
            std::string dst = std::get<2>(*params);
            ClipboardState clip = std::get<3>(*params);
            long long total = std::get<4>(*params); // can be used for progress but not required

            model->m_reporter->hideCalculating();

            if (clip.isCopy) {
                CopyOperation op(src, dst);
                op.execute(model->m_reporter);
            } else {
                MoveOperation op(src, dst);
                op.execute(model->m_reporter);
            }

            // Clear clipboard only if it was a cut operation
if (clip.isCut) {
    model->m_clipboard.isCopy = false;
    model->m_clipboard.isCut = false;
}
// For copy, clipboard stays as is (user can paste again)

if (model->m_observer) {
    model->m_observer->onClipboardChanged(model->m_clipboard);
    model->notifyDirectory();
}

            delete params;
            return 0;
        }, new std::tuple<FileSystemModel*, std::string, std::string, ClipboardState, long long>(
            this, srcFull, destFull, localClip, totalSize
        )); })
        .detach();
}

bool FileSystemModel::renameEntry(const std::string &oldName,
                                  const std::string &newName)
{
    if (::rename(oldName.c_str(), newName.c_str()) != 0)
        return false;

    notifyDirectory();
    return true;
}

void FileSystemModel::deleteEntry(const std::string &name)
{
    char cwd[MAX_PATH];
    getcwd(cwd, sizeof(cwd));
    std::string fullPath = std::string(cwd) + "\\" + name;
    std::string clipFull = m_clipboard.sourceDir + "\\" + m_clipboard.sourceName;
    if (fullPath == clipFull && m_clipboard.hasPending())
    {
        m_clipboard.isCopy = false;
        m_clipboard.isCut = false;
        if (m_observer)
            m_observer->onClipboardChanged(m_clipboard);
    }

    // Show "Calculating size..." dialog
    m_reporter->showCalculating();

    // Launch a thread to compute total size
    std::thread([this, fullPath]()
                {
        long long totalSize = FileOperation::calculateTotalSize(fullPath);

        // Back to main thread to perform the actual delete
        g_idle_add([](gpointer data) -> int {
            auto *params = static_cast<std::tuple<FileSystemModel*, std::string, long long>*>(data);
            FileSystemModel *model = std::get<0>(*params);
            std::string path = std::get<1>(*params);
            long long total = std::get<2>(*params);   // we ignore it, but could be used

            model->m_reporter->hideCalculating();

            DeleteOperation op(path);
            op.execute(model->m_reporter);

            model->notifyDirectory();
            delete params;
            return 0;
        }, new std::tuple<FileSystemModel*, std::string, long long>(this, fullPath, totalSize)); })
        .detach();
}

// ── Search ────────────────────────────────────────────────────────────────────

void FileSystemModel::search(const std::string &query)
{
    if (query.empty() || m_currentPath.empty())
        return;

    gchar *lower = g_utf8_strdown(query.c_str(), -1);
    std::string lowerQuery(lower);
    g_free(lower);

    std::vector<FileEntry> results;
    searchRecursive(m_currentPath, lowerQuery, results);

    if (m_observer)
        m_observer->onSearchResults(results);
}

// ── Private helpers ───────────────────────────────────────────────────────────

std::vector<FileEntry> FileSystemModel::listDirectory(const std::string &path) const
{
    std::vector<FileEntry> entries;
    DIR *dp = opendir(path.c_str());
    if (!dp)
        return entries;

    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr)
    {
        // Skip hidden, system, dot entries.
        DWORD att = GetFileAttributes(entry->d_name);
        if (att == INVALID_FILE_ATTRIBUTES)
            continue;
        if (att & FILE_ATTRIBUTE_HIDDEN)
            continue;
        if (att & FILE_ATTRIBUTE_SYSTEM)
            continue;
        if (strcmp(entry->d_name, ".") == 0)
            continue;
        if (strcmp(entry->d_name, "..") == 0)
            continue;

        char fullPath[MAX_PATH];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s",
                 path.c_str(), entry->d_name);

        struct stat st;
        stat(fullPath, &st);

        FileEntry fe;
        fe.name = entry->d_name;
        fe.fullPath = fullPath;
        fe.isDir = S_ISDIR(st.st_mode) != 0;
        entries.push_back(fe);
    }
    closedir(dp);
    return entries;
}

std::vector<FileEntry> FileSystemModel::listDrives() const
{
    std::vector<FileEntry> drives;
    DWORD mask = GetLogicalDrives();

    for (char letter = 'A'; letter <= 'Z'; ++letter)
    {
        if (!(mask & (1 << (letter - 'A'))))
            continue;

        char path[MAX_PATH + 1] = "";
        char label[MAX_PATH + 1] = "";

        path[0] = letter;
        strcat(path, ":\\");
        GetVolumeInformation(path, label, sizeof(label),
                             nullptr, nullptr, nullptr, nullptr, 0);

        char displayName[MAX_PATH + 10];
        if (label[0] != '\0')
            sprintf(displayName, "%s (%c:)", label, letter);
        else
            sprintf(displayName, "(%c:)", letter);

        FileEntry fe;
        fe.name = displayName;
        fe.fullPath = path;
        fe.isDir = true;
        drives.push_back(fe);
    }
    return drives;
}

void FileSystemModel::searchRecursive(const std::string &dir,
                                      const std::string &lowerQuery,
                                      std::vector<FileEntry> &results) const
{
    DIR *dp = opendir(dir.c_str());
    if (!dp)
        return;

    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr)
    {
        if (strcmp(entry->d_name, ".") == 0)
            continue;
        if (strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir.c_str(), entry->d_name);

        struct stat st;
        stat(path, &st);

        gchar *lowerName = g_utf8_strdown(entry->d_name, -1);
        bool matches = g_strstr_len(lowerName, -1, lowerQuery.c_str()) != nullptr;
        g_free(lowerName);

        if (matches)
        {
            FileEntry fe;
            fe.name = entry->d_name;
            fe.fullPath = path;
            fe.isDir = S_ISDIR(st.st_mode) != 0;
            results.push_back(fe);
        }

        if (S_ISDIR(st.st_mode))
            searchRecursive(path, lowerQuery, results);
    }
    closedir(dp);
}

void FileSystemModel::notifyDirectory()
{
    if (!m_observer)
        return;

    char cwd[MAX_PATH + 1];
    getcwd(cwd, sizeof(cwd));
    m_currentPath = cwd;

    m_observer->onDirectoryChanged(listDirectory(m_currentPath), m_currentPath);
    m_observer->onClipboardChanged(m_clipboard);
}

void FileSystemModel::notifyDrives()
{
    if (m_observer)
        m_observer->onDrivesChanged(listDrives());
}
