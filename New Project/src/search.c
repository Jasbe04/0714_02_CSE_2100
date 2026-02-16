#include "search.h"

/*
 * Recursively searches for files and directories starting from a base path.
 * - Compares file and folder names against the query (case-insensitive).
 * - Adds matching files/folders to the provided GtkListStore with an appropriate icon.
 * - If a directory matches, its contents are also searched recursively.
 *
 * Parameters:
 *   basePath - the directory path to start searching from
 *   query    - the search string to match against file/folder names
 *   store    - the GtkListStore to populate with search results
 *   data     - pointer to Data structure containing app state and icons
 */

void search_files(const char *basePath, const char *query, GtkListStore *store, Data *data)
{
    Data *appData = data;
    DIR *dir = opendir(basePath);
    if (!dir)
        return;

    struct dirent *entry;
    char path[1024];
    struct stat st;

    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
        stat(path, &st);
        gchar *lowerName = g_utf8_strdown(entry->d_name, -1);
        gchar *lowerQuery = g_utf8_strdown(query, -1);

        if (g_strstr_len(lowerName, -1, lowerQuery) != NULL)
        {
            struct stat fstat;
            stat(path, &fstat);
            GdkPixbuf *scaledPixbuf;
            if (S_ISDIR(fstat.st_mode))
            {
                scaledPixbuf = gdk_pixbuf_scale_simple(appData->iconPixbuf, 48, 48, GDK_INTERP_BILINEAR);
            }
            else
            {
                scaledPixbuf = get_file_icon(entry->d_name, 48);
                if (!scaledPixbuf)
                {
                    scaledPixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                                             "text-x-generic", 48, 0, NULL);
                }
            }
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, scaledPixbuf, 1, entry->d_name, 2, path, -1);
            if (scaledPixbuf)
                g_object_unref(scaledPixbuf);
        }
        g_free(lowerName);
        g_free(lowerQuery);
        if (S_ISDIR(st.st_mode))
        {

            search_files(path, query, store, appData);
        }
    }

    closedir(dir);
}

/*
 * Callback function triggered when the Search button is clicked.
 * - Retrieves the search query from the searchEntry widget.
 * - Clears the current list store and starts a recursive search from the current directory.
 * - Populates the GtkListStore with matching files and directories.
 *
 * Parameters:
 *   button - the GTK button that triggered the callback
 *   data   - pointer to Data structure containing app state, search entry, and list store
 */

void on_search_clicked(GtkButton *button, gpointer data)
{
    Data *appData = (Data*)data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(appData->searchEntry));
    if (query[0] == '\0')
        return;
    gtk_list_store_clear(appData->store);
    char currentPath[MAX_PATH] = "";
    getcwd(currentPath, sizeof(currentPath));
    search_files(currentPath, query, appData->store, appData);
}
