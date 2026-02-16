#include "prog.h"

/*
 * Updates the GTK progress bar based on the number of bytes copied.
 * - Computes the fraction of copiedBytes relative to totalBytes.
 * - Updates the progress bar fraction and text label to show percentage.
 * - Ensures the GUI remains responsive by processing pending GTK events.
 *
 * Parameters:
 *   progress - pointer to a CopyProgress structure containing progress state
 */

void update_progress_bar(CopyProgress *progress)
{
    progress->fraction = progress->copiedBytes / progress->totalBytes;
    if (progress->fraction > 1.0)
        progress->fraction = 1.0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->progressBar), progress->fraction);

    gchar *text = g_strdup_printf("%.1f%%", progress->fraction * 100);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->progressBar), text);
    g_free(text);

    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

/*
 * Recursively calculates the total size of a file or directory.
 * - If the path is a directory, sums the sizes of all contained files and subdirectories.
 * - If the path is a file, returns its size in bytes.
 * - Returns 0 if the path is invalid or inaccessible.
 *
 * Parameters:
 *   path - path to the file or directory
 *
 * Returns:
 *   Total size in bytes as a long long integer
 */

long long calculate_total_size(const char *path)
{
    DWORD att = GetFileAttributes(path);
    if (att == INVALID_FILE_ATTRIBUTES)
        return 0;

    long long totalSize = 0;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);
        if (!dp)
            return 0;

        struct dirent *entry;
        char fullPath[4096];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            totalSize += calculate_total_size(fullPath);
        }

        closedir(dp);
    }
    else
    {
        FILE *f = fopen(path, "rb");
        if (!f)
            return 0;

        _fseeki64(f, 0, SEEK_END);
        __int64 fileSize = _ftelli64(f);
        fclose(f);

        totalSize = fileSize;
    }

    return totalSize;
}

/*
 * Callback function to handle closing of a progress window.
 * - Hides the progress window when the user attempts to close it.
 *
 * Parameters:
 *   widget - the GTK widget (progress window)
 *   event  - the GDK event that triggered the callback
 *   data   - user data pointer (unused)
 *
 * Returns:
 *   TRUE to indicate that the event has been handled
 */

gboolean on_progress_close(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_widget_hide(widget);
    return TRUE;
}
