#ifndef PROG_H
#define PROG_H

#include <gtk/gtk.h>
#include "app_data.h"

/**
 * @brief Updates the GTK progress bar with the current copy/move/delete progress.
 *
 * Recalculates the fraction from copied_bytes / total_bytes, clamps it to
 * 1.0, updates the bar's fill and text label, then pumps the GTK event loop
 * so the UI repaints while the file operation is still running.
 *
 * @param progress Pointer to the CopyProgress struct holding all progress state.
 */
void update_progress_bar(CopyProgress *progress);

/**
 * @brief Recursively calculates the total size of a file or directory in bytes.
 *
 * For a regular file the function returns its size.  For a directory it
 * descends the tree and sums the sizes of all contained files.
 *
 * @param path Path to the file or directory to measure.
 * @return long long Total size in bytes, or 0 if the path is invalid or
 *                   cannot be opened.
 */
long long calculate_total_size(const char *path);

/**
 * @brief GTK signal handler that hides the progress window instead of destroying it.
 *
 * Connected to the "delete-event" signal of the progress dialog so that
 * closing the window during an operation simply hides it rather than freeing
 * the widget while it is still in use.
 *
 * @param widget The progress window widget that received the delete event.
 * @param event  The GDK window event (unused).
 * @param data   User data pointer (unused).
 * @return gboolean Always returns TRUE to suppress the default destroy behaviour.
 */
gboolean on_progress_close(GtkWidget *widget, GdkEvent *event, gpointer data);

#endif /* PROG_H */
