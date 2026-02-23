#ifndef FILE_OP_H
#define FILE_OP_H

#include <gtk/gtk.h>
#include "app_data.h"

/**
 * @brief GTK signal handler that prompts the user for a name and creates a
 *        new folder in the current directory.
 *
 * A modal dialog is shown with a text entry.  If the user leaves it blank,
 * "New Folder" is used (auto-numbered if it already exists).  Errors are
 * reported via a message dialog.  The directory listing is refreshed on
 * success.
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void create_new_folder(GtkWidget *widget, GdkEventButton *event,
                       gpointer data);

/**
 * @brief Recursively deletes a file or directory after a cut-paste operation
 *        has completed, updating the progress bar as each item is removed.
 *
 * Unlike delete_function() (which uses remove()/rmdir()), this function uses
 * the Windows API DeleteFile() and RemoveDirectory() so that the same
 * progress bar that tracked the copy phase continues tracking the removal
 * phase.
 *
 * @param path     Absolute path to the source file or directory to remove.
 * @param progress Pointer to the CopyProgress struct used to track progress.
 */
void delete_function_for_cut(const char *path, CopyProgress *progress);

/**
 * @brief Recursively copies a file or directory from @p src to @p dest,
 *        updating the progress bar as each 8 KB chunk is written.
 *
 * Directories are created at the destination with FILE_ATTRIBUTE_NORMAL
 * before their contents are copied recursively.
 *
 * @param src      Absolute path to the source file or directory.
 * @param dest     Absolute path to the destination file or directory.
 * @param progress Pointer to the CopyProgress struct used to track progress.
 */
void paste(const gchar *src, const gchar *dest, CopyProgress *progress);

/**
 * @brief GTK signal handler that executes a pending copy or cut-paste operation
 *        into the current working directory.
 *
 * Displays a progress window, calls paste() to copy the item, and (for a cut
 * operation) calls delete_function_for_cut() to remove the source.  The
 * directory listing is refreshed on completion and the Paste button is dimmed.
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void copy_function(GtkWidget *widget, GdkEventButton *event, gpointer data);

#endif /* FILE_OP_H */
