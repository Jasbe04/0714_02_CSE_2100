/**
 * @file prog.c
 * @brief Implementations for progress-bar helpers and total-size calculation.
 */

#include "prog.h"

#include <gtk/gtk.h>
#include <windows.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

/* ── Progress bar ───────────────────────────────────────────────────────── */

/**
 * @brief Updates the GTK progress bar with the current copy/move/delete progress.
 *
 * @param progress Pointer to the CopyProgress struct holding all progress state.
 */
void update_progress_bar(CopyProgress *progress)
{
    progress->fraction = progress->copied_bytes / progress->total_bytes;
    if (progress->fraction > 1.0)
        progress->fraction = 1.0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->progress_bar),
                                  progress->fraction);

    gchar *text = g_strdup_printf("%.1f%%", progress->fraction * 100);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->progress_bar), text);
    g_free(text);

    /* Pump the event loop so the UI stays responsive during file I/O. */
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

/* ── Size calculation ───────────────────────────────────────────────────── */

/**
 * @brief Recursively calculates the total size of a file or directory in bytes.
 *
 * @param path Path to the file or directory to measure.
 * @return long long Total size in bytes, or 0 if the path is invalid.
 */
long long calculate_total_size(const char *path)
{
    DWORD att = GetFileAttributes(path);
    if (att == INVALID_FILE_ATTRIBUTES)
        return 0;

    long long total_size = 0;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);
        if (!dp)
            return 0;

        struct dirent *entry;
        char           full_path[4096];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(full_path, sizeof(full_path), "%s\\%s",
                     path, entry->d_name);
            total_size += calculate_total_size(full_path);
        }
        closedir(dp);
    }
    else
    {
        FILE *f = fopen(path, "rb");
        if (!f)
            return 0;

        _fseeki64(f, 0, SEEK_END);
        __int64 file_size = _ftelli64(f);
        fclose(f);

        total_size = file_size;
    }

    return total_size;
}

/* ── Progress window close handler ─────────────────────────────────────── */

/**
 * @brief GTK signal handler that hides the progress window instead of destroying it.
 *
 * @param widget The progress window widget that received the delete event.
 * @param event  The GDK window event (unused).
 * @param data   User data pointer (unused).
 * @return gboolean Always returns TRUE to suppress the default destroy behaviour.
 */
gboolean on_progress_close(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_widget_hide(widget);
    return TRUE;
}
