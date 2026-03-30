/**
 * @file search.cpp
 * @brief SRP: recursive file search — nothing else.
 */

#include "search.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

SearchService::SearchService(AppState *state, IconLoader *icons)
    : m_state(state), m_icons(icons)
{}

void SearchService::search(const char *query, const char *basePath)
{
    if (!query || query[0] == '\0') return;

    gtk_list_store_clear(m_state->store);
    searchRecursive(basePath, query);
}

void SearchService::searchRecursive(const char *dir, const char *query)
{
    DIR *dp = opendir(dir);
    if (!dp) return;

    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr)
    {
        if (strcmp(entry->d_name, ".")  == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        struct stat st;
        stat(path, &st);

        // Case-insensitive substring match.
        gchar *lowerName  = g_utf8_strdown(entry->d_name, -1);
        gchar *lowerQuery = g_utf8_strdown(query, -1);
        bool   matches    = g_strstr_len(lowerName, -1, lowerQuery) != nullptr;
        g_free(lowerName);
        g_free(lowerQuery);

        if (matches)
        {
            GdkPixbuf *icon = nullptr;

            if (S_ISDIR(st.st_mode))
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

            GtkTreeIter iter;
            gtk_list_store_append(m_state->store, &iter);
            gtk_list_store_set(m_state->store, &iter,
                               0, icon,
                               1, entry->d_name,
                               2, path,
                               -1);
            if (icon) g_object_unref(icon);
        }

        // Recurse into sub-directories regardless of match.
        if (S_ISDIR(st.st_mode))
            searchRecursive(path, query);
    }

    closedir(dp);
}
