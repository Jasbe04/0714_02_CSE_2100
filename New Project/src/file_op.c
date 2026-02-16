#include "file_op.h"

/*
 * Create a new folder in the current directory.
 * - If no name is entered, it automatically generates "New Folder" or "New Folder (n)".
 * - Displays a GTK dialog for folder name input.
 * - Updates the UI by calling open_directory() after creation.
 * Parameters:
 *   widget - GTK widget that triggered the event
 *   event  - Mouse event (button press)
 *   data   - Pointer to Data structure holding app state
 */

void create_new_folder(GtkWidget *widget, GdkEventButton *event, gpointer data)
{

    Data *appData = (Data *)data;

    if (appData->depthLevel != 0)
    {
        GtkWidget *dialog;
        GtkWidget *entry;
        GtkWidget *area;
        gint response;

        dialog = gtk_dialog_new_with_buttons("New Folder", GTK_WINDOW(appData->window), GTK_DIALOG_MODAL, "Ok", GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
        entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a folder name");
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

                    if (mkdir("New Folder") != 0)
                    {
                        gchar *folder;
                        int index = 1;

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

                        char *errorMessage = strerror(errno);
                        GtkWidget *alert = gtk_message_dialog_new(GTK_WINDOW(appData->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, errorMessage);
                        gtk_dialog_run(GTK_DIALOG(alert));
                        g_free(errorMessage);
                        gtk_widget_destroy(alert);
                        g_free(input);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }

        gtk_widget_destroy(dialog);
        open_directory(appData);
    }
}

/*
 * Recursively deletes the file or directory at `path`.
 * - Used after a cut/move operation.
 * - Updates the cut/move progress using `progress`.
 * - Removes read-only attributes if necessary.
 */

void delete_function_for_cut(const char *path, CopyProgress *progress)
{
    DWORD att = GetFileAttributes(path);

    if (att == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }

    if (att & FILE_ATTRIBUTE_READONLY)
    {
        SetFileAttributes(path, att & ~FILE_ATTRIBUTE_READONLY);
    }

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {
        DIR *dp = opendir(path);

        if (!dp)
        {
            return;
        }

        struct dirent *entry;
        char fullPath[4096];

        while ((entry = readdir(dp)) != NULL)
        {

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            delete_function_for_cut(fullPath, progress);
        }

        closedir(dp);
        RemoveDirectory(path);
    }
    else
    {

        FILE *f = fopen(path, "rb");
        gdouble size = 0;

        if (f)
        {

            _fseeki64(f, 0, SEEK_END);
            size = (_fseeki64(f, 0, SEEK_END));
            fclose(f);
        }

        DeleteFile(path);
        progress->copiedBytes += size;
        update_progress_bar(progress);
    }
}

/*
    this function successfully pastes the source file
*/

void paste(const gchar *src, const gchar *dest, CopyProgress *progress)
{
    DWORD att = GetFileAttributes(src);

    if (att == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }

    if (att & FILE_ATTRIBUTE_DIRECTORY)
    {

        mkdir(dest);
        SetFileAttributes(dest, FILE_ATTRIBUTE_NORMAL);
        DIR *dp = opendir(src);

        if (!dp)
        {
            return;
        }

        struct dirent *entry;
        gchar fullSrc[MAX_PATH + 1];
        gchar fullDest[MAX_PATH + 1];

        while ((entry = readdir(dp)) != NULL)
        {

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            snprintf(fullSrc, sizeof(fullSrc), "%s\\%s", src, entry->d_name);
            snprintf(fullDest, sizeof(fullDest), "%s\\%s", dest, entry->d_name);
            paste(fullSrc, fullDest, progress);
        }

        closedir(dp);
    }
    else
    {
        FILE *source = fopen(src, "rb");
        FILE *target = fopen(dest, "wb");

        if (!source || !target)
        {
            return;
        }

        char buffer[8192];
        size_t bytesRead;

        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {

            size_t bytesWritten = fwrite(buffer, 1, bytesRead, target);
            progress->copiedBytes += bytesWritten;
            progress->fraction = (double)progress->copiedBytes / progress->totalBytes;
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->progressBar), progress->fraction);
            gchar *text = g_strdup_printf("%.1f%%", progress->fraction * 100);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->progressBar), text);
            g_free(text);

            while (gtk_events_pending())
            {
                gtk_main_iteration_do(FALSE);
            }
        }

        fclose(source);
        fclose(target);
    }
}

/*
 * Handles the "Paste" button click event.
 * - Sets up a GTK progress window and progress bar.
 * - Calls paste() to copy files/folders.
 * - If in cut mode, deletes the source after copying.
 * - Updates the UI and resets cut/copy flags.
 */

void copy_function(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    Data *appData = (Data *)data;

    if (appData->isCutMode == 1 || appData->isCopyMode == 1)
    {

        GtkWidget *progressWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(progressWindow), appData->isCopyMode ? "Copying..." : "Moving...");
        gtk_window_set_default_size(GTK_WINDOW(progressWindow), 400, 60);
        gtk_window_set_position(GTK_WINDOW(progressWindow), GTK_WIN_POS_CENTER);
        gtk_container_set_border_width(GTK_CONTAINER(progressWindow), 10);
        gtk_window_set_resizable(GTK_WINDOW(progressWindow), FALSE);
        gtk_window_set_type_hint(GTK_WINDOW(progressWindow), GDK_WINDOW_TYPE_HINT_DIALOG);

        GtkWidget *progressBar = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progressBar), TRUE);
        gtk_container_add(GTK_CONTAINER(progressWindow), progressBar);
        gtk_widget_show_all(progressWindow);

        gchar src[MAX_PATH + 1], dest[MAX_PATH + 1], destFolder[MAX_PATH + 1];
        getcwd(destFolder, sizeof(destFolder));
        snprintf(src, sizeof(src), "%s\\%s", appData->pathWay1, appData->fName1);
        snprintf(dest, sizeof(dest), "%s\\%s", destFolder, appData->fName1);

        CopyProgress prog = {0};
        prog.progressBar = progressBar;
        prog.totalBytes = calculate_total_size(src);
        prog.copiedBytes = 0;
        prog.fraction = 0.0;

        paste(src, dest, &prog);

        if (appData->isCutMode == 1)
        {
            delete_function_for_cut(src, &prog);
        }

        update_progress_bar(&prog);
        gtk_widget_destroy(progressWindow);
        appData->isCutMode = 0;
        gtk_widget_set_opacity(appData->pasteEvent, 0.1);
        open_directory(appData);
    }
}
