/**
 * @file file_op.cpp
 * @brief Implementations for CopyOperation, MoveOperation, DeleteOperation.
 *
 * SOLID: OCP — each operation is a self-contained class; adding a new one
 *              (e.g. CompressOperation) needs no changes here.
 *        LSP — all three are drop-in replacements for FileOperation*.
 */

#include "file_op.hpp"

#include <windows.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <string>

// ── FileOperation (base) ──────────────────────────────────────────────────────

long long FileOperation::calculateTotalSize(const std::string &path)
{
    DWORD att = GetFileAttributes(path.c_str());
    if (att == INVALID_FILE_ATTRIBUTES)
        return 0;

    long long total = 0;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path.c_str());
        if (!dp) return 0;

        struct dirent *entry;
        while ((entry = readdir(dp)) != nullptr)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;
            total += calculateTotalSize(path + "\\" + entry->d_name);
        }
        closedir(dp);
    }
    else
    {
        FILE *f = fopen(path.c_str(), "rb");
        if (!f) return 0;
        _fseeki64(f, 0, SEEK_END);
        total = (long long)_ftelli64(f);
        fclose(f);
    }
    return total;
}

// ── CopyOperation ─────────────────────────────────────────────────────────────

CopyOperation::CopyOperation(const std::string &src, const std::string &dest)
    : m_src(src), m_dest(dest)
{}

void CopyOperation::execute(IProgressReporter *reporter)
{
    long long total  = calculateTotalSize(m_src);
    long long copied = 0;
    if (total == 0) total = 1; // avoid division by zero

    reporter->show(operationName());
    copyRecursive(m_src, m_dest, total, copied, reporter);
    reporter->update(1.0);
    reporter->hide();
}

void CopyOperation::copyRecursive(const std::string &src,
                                   const std::string &dest,
                                   long long          total,
                                   long long         &copied,
                                   IProgressReporter *reporter)
{
    DWORD att = GetFileAttributes(src.c_str());
    if (att == INVALID_FILE_ATTRIBUTES)
        return;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        mkdir(dest.c_str());
        SetFileAttributes(dest.c_str(), FILE_ATTRIBUTE_NORMAL);

        DIR *dp = opendir(src.c_str());
        if (!dp) return;

        struct dirent *entry;
        while ((entry = readdir(dp)) != nullptr)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            copyRecursive(src  + "\\" + entry->d_name,
                          dest + "\\" + entry->d_name,
                          total, copied, reporter);
        }
        closedir(dp);
    }
    else
    {
        FILE *source = fopen(src.c_str(),  "rb");
        FILE *target = fopen(dest.c_str(), "wb");
        if (!source || !target)
        {
            if (source) fclose(source);
            if (target) fclose(target);
            return;
        }

        char   buffer[8192];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {
            size_t written = fwrite(buffer, 1, bytesRead, target);
            copied += (long long)written;
            reporter->update((double)copied / (double)total);
        }
        fclose(source);
        fclose(target);
    }
}

// ── MoveOperation ─────────────────────────────────────────────────────────────

MoveOperation::MoveOperation(const std::string &src, const std::string &dest)
    : m_src(src), m_dest(dest)
{}

void MoveOperation::execute(IProgressReporter *reporter)
{
    // SRP: delegate copy to CopyOperation, then delegate delete to DeleteOperation.
    // We split the total size in two halves for progress: 50% copy, 50% delete.
    long long total  = calculateTotalSize(m_src);
    long long copied = 0;
    if (total == 0) total = 1;

    reporter->show(operationName());

    // --- Copy phase ---
    CopyOperation copyOp(m_src, m_dest);
    copyOp.execute(reporter);  // shows its own progress window briefly

    // --- Delete phase ---
    long long deleted = 0;
    DeleteOperation::deleteRecursive(m_src, total, deleted, reporter);

    reporter->update(1.0);
    reporter->hide();
}

// ── DeleteOperation ───────────────────────────────────────────────────────────

DeleteOperation::DeleteOperation(const std::string &path)
    : m_path(path)
{}

void DeleteOperation::execute(IProgressReporter *reporter)
{
    long long total   = calculateTotalSize(m_path);
    long long deleted = 0;
    if (total == 0) total = 1;

    reporter->show(operationName());
    deleteRecursive(m_path, total, deleted, reporter);
    reporter->update(1.0);
    reporter->hide();
}

/*static*/
void DeleteOperation::deleteRecursive(const std::string &path,
                                       long long          total,
                                       long long         &deleted,
                                       IProgressReporter *reporter)
{
    DWORD att = GetFileAttributes(path.c_str());
    if (att == INVALID_FILE_ATTRIBUTES)
        return;

    if (att & FILE_ATTRIBUTE_READONLY)
        SetFileAttributes(path.c_str(), att & ~FILE_ATTRIBUTE_READONLY);

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path.c_str());
        if (!dp) return;

        struct dirent *entry;
        while ((entry = readdir(dp)) != nullptr)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;
            deleteRecursive(path + "\\" + entry->d_name, total, deleted, reporter);
        }
        closedir(dp);
        RemoveDirectory(path.c_str());
    }
    else
    {
        // Measure the file before deleting it.
        FILE *f = fopen(path.c_str(), "rb");
        if (f)
        {
            _fseeki64(f, 0, SEEK_END);
            deleted += (long long)_ftelli64(f);
            fclose(f);
        }
        DeleteFile(path.c_str());
        reporter->update((double)deleted / (double)total);
    }
}
