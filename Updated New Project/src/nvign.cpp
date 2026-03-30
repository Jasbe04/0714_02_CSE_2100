/**
 * @file nvign.cpp
 * @brief SRP: directory navigation — nothing else.
 *        ISP: implements INavigable only.
 */

#include "nvign.hpp"

#include <gtk/gtk.h>
#include <windows.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

NavigationManager::NavigationManager(AppState *state, IconLoader *icons)
    : m_state(state), m_icons(icons)
{}

// ── showDrives ────────────────────────────────────────────────────────────────

void NavigationManager::showDrives()
{
    gtk_widget_set_opacity(m_state->event1, 0.1);
    gtk_widget_set_opacity(m_state->event3, 0.1);
    gtk_widget_set_opacity(m_state->event4, 0.1);

    if (gtk_widget_get_parent(m_state->boxSearch))
        gtk_widget_hide(m_state->boxSearch);

    gtk_label_set_text(GTK_LABEL(m_state->labelPath), "Devices and Drives");
    gtk_widget_set_name(m_state->labelPath, "labelPath");

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        provider,
        "#labelPath { font-size: 20px; font-family: Arial; font-weight: bold; }",
        -1, nullptr);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_list_store_clear(m_state->store);

    GtkTreeIter iter;
    DWORD drives = GetLogicalDrives();

    for (char drive = 'A'; drive <= 'Z'; ++drive)
    {
        if (!(drives & (1 << (drive - 'A'))))
            continue;

        char path[MAX_PATH + 1]      = "";
        char name1[MAX_PATH + 1]     = "";
        char realName[MAX_PATH + 1]  = "";

        path[0] = drive;
        strcat(path, ":\\");

        GetVolumeInformation(path, name1, sizeof(name1),
                             nullptr, nullptr, nullptr, nullptr, 0);
        if (name1[0] != '\0')
            strcat(name1, " ");

        char driveLetter[3] = { drive, ':', '\0' };
        sprintf(realName, "%s(%s)", name1, driveLetter);

        gtk_list_store_append(m_state->store, &iter);
        gtk_list_store_set(m_state->store, &iter,
                           0, m_state->scaledDriveIcon,
                           1, realName,
                           -1);
    }
}

// ── openDirectory ─────────────────────────────────────────────────────────────

void NavigationManager::openDirectory()
{
    gtk_box_pack_start(GTK_BOX(m_state->fullBox), m_state->boxSearch,
                       FALSE, FALSE, 0);

    if (m_state->isCopy || m_state->isCut)
        gtk_widget_set_opacity(m_state->event4, 1.0);

    if (m_state->navForward == 0)
        gtk_widget_set_opacity(m_state->event2, 0.1);

    gtk_widget_set_opacity(m_state->event1, 1.0);
    gtk_widget_set_opacity(m_state->event3, 1.0);

    gtk_list_store_clear(m_state->store);

    char cwd[MAX_PATH + 1];
    getcwd(cwd, sizeof(cwd));

    char pathLabel[MAX_PATH + 60];
    sprintf(pathLabel, "Current path: %s", cwd);
    gtk_label_set_text(GTK_LABEL(m_state->labelPath), pathLabel);
    gtk_widget_show_all(m_state->parentBox);

    DIR   *dp    = opendir(".");
    int    count = 0;
    GtkTreeIter iter;

    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr)
    {
        DWORD att = GetFileAttributes(entry->d_name);
        if (att == INVALID_FILE_ATTRIBUTES)         continue;
        if (att & FILE_ATTRIBUTE_HIDDEN)             continue;
        if (att & FILE_ATTRIBUTE_SYSTEM)             continue;
        if (strcmp(entry->d_name, ".")  == 0)        continue;
        if (strcmp(entry->d_name, "..") == 0)        continue;

        ++count;

        char fullPath[MAX_PATH];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", cwd, entry->d_name);

        struct stat    fileStat;
        stat(fullPath, &fileStat);
        GdkPixbuf     *icon = nullptr;

        if (S_ISDIR(fileStat.st_mode))
        {
            icon = IconLoader::scale(m_state->iconPixbuf, 48);
        }
        else
        {
            icon = m_icons->getFileIcon(entry->d_name, 48);
            if (!icon)
                icon = gtk_icon_theme_load_icon(
                    gtk_icon_theme_get_default(),
                    "text-x-generic", 48, (GtkIconLookupFlags)0, nullptr);
        }

        gtk_list_store_append(m_state->store, &iter);
        gtk_list_store_set(m_state->store, &iter,
                           0, icon,
                           1, entry->d_name,
                           2, fullPath,
                           -1);
        if (icon) g_object_unref(icon);
    }
    closedir(dp);

    // Swap the scrolled-window child based on whether the directory is empty.
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(m_state->scrolledWindow));
    if (child)
        gtk_container_remove(GTK_CONTAINER(m_state->scrolledWindow), child);

    if (count == 0)
    {
        GtkWidget *empty = gtk_label_new("This folder is empty");
        gtk_container_add(GTK_CONTAINER(m_state->scrolledWindow), empty);
    }
    else
    {
        gtk_container_add(GTK_CONTAINER(m_state->scrolledWindow), m_state->treeview);
    }

    gtk_widget_show_all(m_state->scrolledWindow);
}

// ── goToParent ────────────────────────────────────────────────────────────────

void NavigationManager::goToParent()
{
    if (m_state->depth == 0) return;

    gtk_widget_set_opacity(m_state->event2, 1.0);

    // Save CWD onto the forward stack before moving up.
    char cwd[MAX_PATH + 1];
    getcwd(cwd, sizeof(cwd));
    m_state->forwardStack.push(std::string(cwd));

    --m_state->depth;
    ++m_state->navForward;

    if (m_state->depth == 0)
    {
        gtk_list_store_clear(m_state->store);
        showDrives();
    }
    else
    {
        std::string prev = m_state->backStack.top();
        m_state->backStack.pop();
        chdir(prev.c_str());
        openDirectory();
    }
}

// ── goForward ─────────────────────────────────────────────────────────────────

void NavigationManager::goForward()
{
    if (m_state->navForward == 0 || m_state->forwardStack.empty())
        return;

    // Save CWD onto the backward stack before moving forward.
    char cwd[MAX_PATH + 1];
    getcwd(cwd, sizeof(cwd));
    m_state->backStack.push(std::string(cwd));

    std::string next = m_state->forwardStack.top();
    m_state->forwardStack.pop();

    --m_state->navForward;
    ++m_state->depth;

    chdir(next.c_str());
    openDirectory();
}
