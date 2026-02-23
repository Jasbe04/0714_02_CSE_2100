#ifndef MENU_OP_H
#define MENU_OP_H

#include <gtk/gtk.h>
#include "app_data.h"

/**
 * @brief GTK signal handler that prompts the user to rename the selected file
 *        or folder.
 *
 * Retrieves the current name from the tree model, shows a modal dialog with
 * a text entry pre-focused, and calls rename(2) on confirmation.  The
 * directory listing is refreshed on success.
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void folder_rename(GtkMenuItem *item, gpointer data);

/**
 * @brief Recursively deletes a file or directory, updating the progress bar
 *        after each individual file is removed.
 *
 * Read-only attributes are cleared before deletion is attempted.  Failure to
 * remove a file or directory is reported via a MessageBox.
 *
 * @param path     Absolute path to the file or directory to delete.
 * @param progress Pointer to the CopyProgress struct used to track progress.
 */
void delete_function(const gchar *path, CopyProgress *progress);

/**
 * @brief GTK signal handler that asks the user to confirm and then permanently
 *        deletes the selected file or folder.
 *
 * Shows a yes/no confirmation dialog.  On confirmation a progress window is
 * displayed, delete_function() is called, and the directory listing is
 * refreshed on completion.
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void delete(GtkMenuItem *item, gpointer data);

/**
 * @brief GTK signal handler that marks the selected item for copying.
 *
 * Sets is_copy = 1 and is_cut = 0, records the source path and filename in
 * the Data struct, and makes the Paste button fully opaque.
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void copy(GtkMenuItem *item, gpointer data);

/**
 * @brief GTK signal handler that marks the selected item for cutting (move).
 *
 * Sets is_cut = 1 and is_copy = 0, records the source path and filename in
 * the Data struct, and makes the Paste button fully opaque.
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void cut(GtkMenuItem *item, gpointer data);

#endif /* MENU_OP_H */
