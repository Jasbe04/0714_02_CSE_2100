#ifndef SEARCH_H
#define SEARCH_H

#include <gtk/gtk.h>
#include "app_data.h"

/**
 * @brief Recursively searches a directory tree for entries whose names contain
 *        the given query string (case-insensitive) and appends matches to the
 *        list store.
 *
 * @param base_path Root directory from which the search begins.
 * @param query     Substring to match against file and folder names.
 * @param store     GTK list store to which matching entries are appended.
 * @param data      Pointer to the Data struct, used to access the folder icon
 *                  pixbuf for directory entries.
 */
void search_files(const char *base_path, const char *query,
                  GtkListStore *store, Data *data);

/**
 * @brief GTK signal handler triggered when the Search button is clicked.
 *
 * Reads the query from the search entry widget, clears the list store, and
 * calls search_files() rooted at the current working directory.
 *
 * @param button The button widget that was clicked (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void on_search_clicked(GtkButton *button, gpointer data);

#endif /* SEARCH_H */
