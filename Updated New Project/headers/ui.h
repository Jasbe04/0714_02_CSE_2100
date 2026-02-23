#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "app_data.h"

/**
 * @brief GTK signal handler triggered when the user double-clicks a row in the
 *        tree view.
 *
 * For depth == 0 (drive list): extracts the drive letter and changes the
 * working directory to that drive root.  For depth > 0: stats the selected
 * name; if it is a directory the backward stack is updated and the directory
 * is entered; if it is a regular file it is opened with ShellExecute().
 *
 * @param treeview The GtkTreeView that received the row-activated signal.
 * @param path     Tree path of the activated row.
 * @param col      The activated column (unused).
 * @param data     Pointer to the Data struct cast to gpointer.
 */
void select_directory(GtkTreeView *treeview, GtkTreePath *path,
                      GtkTreeViewColumn *col, gpointer data);

/**
 * @brief GTK signal handler for mouse button presses on the tree view.
 *
 * Shows a right-click context menu (Rename / Copy / Cut / Delete) when the
 * user right-clicks a selected row while depth > 0.
 *
 * @param treeview The GtkTreeView that received the button-press event.
 * @param event    The GDK button event with coordinates and button number.
 * @param data     Pointer to the Data struct cast to gpointer.
 * @return gboolean TRUE if a context menu was shown, FALSE otherwise.
 */
gboolean right_button_click(GtkTreeView *treeview, GdkEventButton *event,
                             gpointer data);

/**
 * @brief Builds and displays the complete file-manager UI.
 *
 * Creates the main window, toolbar (Back / Next / New-Folder / Paste buttons),
 * path label, search bar, scrolled tree view, and connects all GTK signals.
 * Calls show_drives() to populate the initial drive list and starts the GTK
 * main event loop.
 *
 * @param data Pointer to the heap-allocated Data struct to populate and use.
 */
void set_ui(Data *data);

#endif /* UI_H */
