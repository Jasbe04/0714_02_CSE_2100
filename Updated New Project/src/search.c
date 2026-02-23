/**
 * @file search.c
 * @brief Implementations for recursive file search and the search-button handler.
 */

#include "search.h"
#include "icon.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <windows.h>

/* ── Recursive file search ──────────────────────────────────────────────── */

/**
 * @brief Recursively searches a directory tree for entries matching a query string.
 *
 * @param base_path Root directory from which the search begins.
 * @param query     Substring to match against file and folder names (case-insensitive).
 * @param store     GTK list store to which matching entries are appended.
 * @param data      Pointer to the Data struct for accessing the folder icon pixbuf.
 */
void search_files(const char *base_path, const char *query,
                  GtkListStore *store, Data *data)
{
    DIR *dir = opendir(base_path);
    if (!dir)
        return;

    struct dirent *entry;
    char           path[1024];
    struct stat    st;

    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
        stat(path, &st);

        gchar *lower_name  = g_utf8_strdown(entry->d_name, -1);
        gchar *lower_query = g_utf8_strdown(query, -1);

        if (g_strstr_len(lower_name, -1, lower_query) != NULL)
        {
            struct stat   file_stat;
            stat(path, &file_stat);
            GdkPixbuf    *scaled_pixbuf;

            if (S_ISDIR(file_stat.st_mode))
            {
                /* Use the application folder icon for matched directories. */
                scaled_pixbuf = gdk_pixbuf_scale_simple(
                    data->icon_pixbuf, 48, 48, GDK_INTERP_BILINEAR);
            }
            else
            {
                scaled_pixbuf = get_file_icon(entry->d_name, 48);
                if (!scaled_pixbuf)
                    scaled_pixbuf = gtk_icon_theme_load_icon(
                        gtk_icon_theme_get_default(),
                        "text-x-generic", 48, 0, NULL);
            }

            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, scaled_pixbuf,
                               1, entry->d_name,
                               2, path,
                               -1);
            if (scaled_pixbuf)
                g_object_unref(scaled_pixbuf);
        }

        g_free(lower_name);
        g_free(lower_query);

        /* Recurse into sub-directories regardless of whether they matched. */
        if (S_ISDIR(st.st_mode))
            search_files(path, query, store, data);
    }

    closedir(dir);
}

/* ── Search-button handler ──────────────────────────────────────────────── */

/**
 * @brief GTK signal handler triggered when the Search button is clicked.
 *
 * @param button The button widget that was clicked (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void on_search_clicked(GtkButton *button, gpointer data)
{
    Data       *app_data = (Data *)data;
    const char *query    = gtk_entry_get_text(GTK_ENTRY(app_data->search_entry));

    if (query[0] == '\0')
        return;

    gtk_list_store_clear(app_data->store);

    char current_path[MAX_PATH] = "";
    getcwd(current_path, sizeof(current_path));
    search_files(current_path, query, app_data->store, app_data);
}
