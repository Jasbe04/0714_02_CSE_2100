/**
 * @file file_op.c
 * @brief Implementations for file operations: create folder, paste (copy),
 *        delete-for-cut, and the top-level copy/move function.
 */

#include "file_op.h"
#include "prog.h"
#include "nvign.h"

#include <gtk/gtk.h>
#include <windows.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

/* ── Create new folder ──────────────────────────────────────────────────── */

/**
 * @brief GTK signal handler that prompts the user for a name and creates a
 *        new folder in the current directory.
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void create_new_folder(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    Data *app_data = (Data *)data;

    if (app_data->depth != 0)
    {
        GtkWidget *dialog;
        GtkWidget *entry;
        GtkWidget *area;
        gint       response;

        dialog = gtk_dialog_new_with_buttons(
            "New Folder", GTK_WINDOW(app_data->window), GTK_DIALOG_MODAL,
            "Ok", GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
        entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry),
                                       "Enter a folder name");
        area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_add(GTK_CONTAINER(area), entry);

        while (1)
        {
            gtk_widget_show_all(dialog);
            response = gtk_dialog_run(GTK_DIALOG(dialog));
            const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

            if (response == GTK_RESPONSE_OK)
            {
                gchar *input = g_strdup(text);
                if (input[0] == '\0')
                {
                    /* No name entered – use "New Folder", auto-number on collision. */
                    if (mkdir("New Folder") != 0)
                    {
                        gchar *folder;
                        int    index = 1;
                        while (1)
                        {
                            folder = g_strdup_printf("New folder (%d)", index);
                            if (mkdir(folder) == 0)
                            {
                                g_free(folder);
                                break;
                            }
                            index++;
                            g_free(folder);
                        }
                    }
                    g_free(input);
                    break;
                }
                else
                {
                    if (mkdir(input) != 0)
                    {
                        char      *error_message = strerror(errno);
                        GtkWidget *alert = gtk_message_dialog_new(
                            GTK_WINDOW(app_data->window), GTK_DIALOG_MODAL,
                            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error_message);
                        gtk_dialog_run(GTK_DIALOG(alert));
                        gtk_widget_destroy(alert);
                        g_free(input);
                    }
                    else
                        break;
                }
            }
            else
                break;
        }

        gtk_widget_destroy(dialog);
        open_directory(app_data);
    }
}

/* ── Delete helper for cut (recursive) ─────────────────────────────────── */

/**
 * @brief Recursively deletes a file or directory after a cut-paste operation
 *        has completed.
 *
 * @param path     Absolute path to the source file or directory to remove.
 * @param progress Pointer to the CopyProgress struct used to track progress.
 */
void delete_function_for_cut(const char *path, CopyProgress *progress)
{
    DWORD att = GetFileAttributes(path);
    if (att == INVALID_FILE_ATTRIBUTES)
        return;

    if (att & FILE_ATTRIBUTE_READONLY)
        SetFileAttributes(path, att & ~FILE_ATTRIBUTE_READONLY);

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);
        if (!dp)
            return;

        struct dirent *entry;
        char           full_path[4096];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(full_path, sizeof(full_path), "%s\\%s",
                     path, entry->d_name);
            delete_function_for_cut(full_path, progress);
        }
        closedir(dp);
        RemoveDirectory(path);
    }
    else
    {
        FILE   *f    = fopen(path, "rb");
        gdouble size = 0;
        if (f)
        {
            _fseeki64(f, 0, SEEK_END);
            size = (gdouble)_ftelli64(f);
            fclose(f);
        }
        DeleteFile(path);
        progress->copied_bytes += size;
        update_progress_bar(progress);
    }
}

/* ── Paste helper (recursive copy) ─────────────────────────────────────── */

/**
 * @brief Recursively copies a file or directory from @p src to @p dest,
 *        updating the progress bar as each chunk is written.
 *
 * @param src      Absolute path to the source file or directory.
 * @param dest     Absolute path to the destination file or directory.
 * @param progress Pointer to the CopyProgress struct used to track progress.
 */
void paste(const gchar *src, const gchar *dest, CopyProgress *progress)
{
    DWORD att = GetFileAttributes(src);
    if (att == INVALID_FILE_ATTRIBUTES)
        return;

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        mkdir(dest);
        SetFileAttributes(dest, FILE_ATTRIBUTE_NORMAL);

        DIR *dp = opendir(src);
        if (!dp)
            return;

        struct dirent *entry;
        gchar          full_src[MAX_PATH + 1];
        gchar          full_dest[MAX_PATH + 1];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(full_src,  sizeof(full_src),  "%s\\%s",
                     src,  entry->d_name);
            snprintf(full_dest, sizeof(full_dest), "%s\\%s",
                     dest, entry->d_name);
            paste(full_src, full_dest, progress);
        }
        closedir(dp);
    }
    else
    {
        FILE  *source = fopen(src,  "rb");
        FILE  *target = fopen(dest, "wb");
        if (!source || !target)
            return;

        char   buffer[8192];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {
            size_t bytes_written = fwrite(buffer, 1, bytes_read, target);
            progress->copied_bytes += bytes_written;

            gdouble fraction = (double)progress->copied_bytes /
                               progress->total_bytes;
            gtk_progress_bar_set_fraction(
                GTK_PROGRESS_BAR(progress->progress_bar), fraction);

            gchar *text = g_strdup_printf("%.1f%%", fraction * 100);
            gtk_progress_bar_set_text(
                GTK_PROGRESS_BAR(progress->progress_bar), text);
            g_free(text);

            while (gtk_events_pending())
                gtk_main_iteration_do(FALSE);
        }
        fclose(source);
        fclose(target);
    }
}

/* ── Top-level copy / move operation ────────────────────────────────────── */

/**
 * @brief GTK signal handler that executes a pending copy or cut-paste operation
 *        into the current working directory.
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void copy_function(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    Data *app_data = (Data *)data;

    if (app_data->is_cut == 1 || app_data->is_copy == 1)
    {
        GtkWidget *progress_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(progress_window),
                             app_data->is_copy ? "Copying..." : "Moving...");
        gtk_window_set_default_size(GTK_WINDOW(progress_window), 400, 60);
        gtk_window_set_position(GTK_WINDOW(progress_window),
                                GTK_WIN_POS_CENTER);
        gtk_container_set_border_width(GTK_CONTAINER(progress_window), 10);
        gtk_window_set_resizable(GTK_WINDOW(progress_window), FALSE);
        gtk_window_set_type_hint(GTK_WINDOW(progress_window),
                                 GDK_WINDOW_TYPE_HINT_DIALOG);

        GtkWidget *progress_bar = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
        gtk_container_add(GTK_CONTAINER(progress_window), progress_bar);
        gtk_widget_show_all(progress_window);

        gchar src[MAX_PATH + 1];
        gchar dest[MAX_PATH + 1];
        gchar dest_folder[MAX_PATH + 1];
        getcwd(dest_folder, sizeof(dest_folder));
        snprintf(src,  sizeof(src),  "%s\\%s",
                 app_data->pathway1, app_data->fname1);
        snprintf(dest, sizeof(dest), "%s\\%s",
                 dest_folder, app_data->fname1);

        CopyProgress prog = {0};
        prog.progress_bar = progress_bar;
        prog.total_bytes  = calculate_total_size(src);
        prog.copied_bytes = 0;
        prog.fraction     = 0.0;

        paste(src, dest, &prog);

        if (app_data->is_cut == 1)
            delete_function_for_cut(src, &prog);

        update_progress_bar(&prog);
        gtk_widget_destroy(progress_window);

        app_data->is_cut = 0;
        gtk_widget_set_opacity(app_data->event4, 0.1);
        open_directory(app_data);
    }
}
