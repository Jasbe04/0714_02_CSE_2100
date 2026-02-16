#include "menu_op.h"

/*
 * Renames a selected folder.
 * - Opens a modal dialog asking the user to input a new name.
 * - If the user confirms, attempts to rename the folder on disk.
 * - Shows an error message if renaming fails.
 * - Refreshes the file manager view after renaming.
 *
 * Parameters:
 *   item - the GtkMenuItem that was activated
 *   data - pointer to the main Data structure (app state)
 */

void folder_rename(GtkMenuItem *item, gpointer data)
{
    Data *appData = (Data*)data;
    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget *area;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(appData->treeview));
    gtk_tree_model_get_iter(model, &iter, appData->path);
    gchar *folderName = NULL;
    gtk_tree_model_get(model, &iter, 1, &folderName, -1);
    dialog = gtk_dialog_new_with_buttons("Rename", GTK_WINDOW(appData->window), GTK_DIALOG_MODAL, "Ok", GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a new name");
    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(area), entry);
    while (1)
    {
        gint response;
        gtk_widget_show_all(dialog);
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_OK)
        {
            const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));
            if (name[0] != '\0')
            {
                if (rename(folderName, name) != 0)
                {
                    gchar *errorMessage = strerror(errno);
                    GtkWidget *alert = gtk_message_dialog_new(GTK_WINDOW(appData->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, errorMessage);
                    gtk_dialog_run(GTK_DIALOG(alert));
                    g_free(errorMessage);
                    gtk_widget_destroy(alert);
                }
                else
                {
                    open_directory(appData);
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
            break;
    }
    g_free(folderName);
    gtk_widget_destroy(dialog);
}

/*
 * Recursively deletes a file or folder.
 * - Checks file attributes and removes read-only if necessary.
 * - If the path is a directory, deletes all contents recursively.
 * - Updates the progress bar as files are deleted.
 *
 * Parameters:
 *   path     - path of the file or folder to delete
 *   progress - pointer to a CopyProgress struct to track deletion progress
 */

void delete_function(const gchar *path, CopyProgress *progress)
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
        gchar fullPath[MAX_PATH + 1];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
            delete_function(fullPath, progress);
        }

        closedir(dp);

        if (rmdir(path))
        {
            gchar msg[256];
            snprintf(msg, sizeof(msg), "Failed to remove directory: %s", path);
            MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
        }
    }
    else
    {
        FILE *f = fopen(path, "rb");
        long long file_size = 0;
        if (f)
        {
            _fseeki64(f, 0, SEEK_END);
            file_size = _ftelli64(f);
            fclose(f);
        }

        if (remove(path))
        {
            gchar msg[256];
            snprintf(msg, sizeof(msg), "Failed to delete file: %s", path);
            MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
        }
        else
        {
            progress->copiedBytes += file_size;
            update_progress_bar(progress);
        }
    }
}

/*
 * Deletes the currently selected file or folder.
 * - Confirms deletion via a dialog box.
 * - Opens a temporary progress window to show deletion progress.
 * - Calls delete_function() recursively for actual deletion.
 * - Refreshes the file manager view after deletion.
 *
 * Parameters:
 *   item - the GtkMenuItem that was activated
 *   data - pointer to the main Data structure (app state)
 */

void delete(GtkMenuItem *item, gpointer data)
{
    Data *appData = (Data *)data;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(appData->treeview));

    if (!gtk_tree_model_get_iter(model, &iter, appData->path))
        return;

    gchar *fName = NULL;
    gtk_tree_model_get(model, &iter, 1, &fName, -1);

    if (!fName)
        return;

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(appData->window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "Are you sure you want to delete '%s'?", fName);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response != GTK_RESPONSE_YES)
    {
        g_free(fName);
        return;
    }

    GtkWidget *progressWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(progressWindow), "Deleting...");
    gtk_window_set_default_size(GTK_WINDOW(progressWindow), 400, 60);
    gtk_window_set_position(GTK_WINDOW(progressWindow), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(progressWindow), 10);
    gtk_window_set_resizable(GTK_WINDOW(progressWindow), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(progressWindow), GDK_WINDOW_TYPE_HINT_DIALOG);

    GtkWidget *progressBar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progressBar), TRUE);
    gtk_container_add(GTK_CONTAINER(progressWindow), progressBar);
    gtk_widget_show_all(progressWindow);

    CopyProgress prog = {0};
    prog.progressBar = progressBar;
    prog.totalBytes = calculate_total_size(fName);
    prog.copiedBytes = 0;
    prog.fraction = 0.0;

    char fullPath[MAX_PATH];
    getcwd(fullPath, sizeof(fullPath));
    strncat(fullPath, "\\", sizeof(fullPath) - strlen(fullPath) - 1);
    strncat(fullPath, fName, sizeof(fullPath) - strlen(fullPath) - 1);

    delete_function(fullPath, &prog);

    update_progress_bar(&prog);

    gtk_widget_destroy(progressWindow);
    open_directory(appData);

    g_free(fName);
}

/*
 * Marks the currently selected item for copy.
 * - Sets the application state to copy mode.
 * - Records the current path and file/folder name for pasting later.
 * - Enables the paste button.
 *
 * Parameters:
 *   item - the GtkMenuItem that was activated
 *   data - pointer to the main Data structure (app state)
 */

void copy(GtkMenuItem *item, gpointer data)
{
    Data *appData = (Data *)data;
    appData->isCopyMode = 1;
    appData->isCutMode = 0;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(appData->treeview));
    if (!gtk_tree_model_get_iter(model, &iter, appData->path))
        return;
    getcwd(appData->pathWay1, sizeof(appData->pathWay1));
    gchar *gname = NULL;
    gtk_tree_model_get(model, &iter, 1, &gname, -1);
    snprintf(appData->fName1, sizeof(appData->fName1), "%s", gname);
    gtk_widget_set_opacity(appData->pasteEvent, 1);
}

/*
 * Marks the currently selected item for copy.
 * - Sets the application state to copy mode.
 * - Records the current path and file/folder name for pasting later.
 * - Enables the paste button.
 *
 * Parameters:
 *   item - the GtkMenuItem that was activated
 *   data - pointer to the main Data structure (app state)
 */

void cut(GtkMenuItem *item, gpointer data)
{
    Data *appData = (Data *)data;
    appData->isCopyMode = 0;
    appData->isCutMode = 1;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(appData->treeview));
    if (!gtk_tree_model_get_iter(model, &iter, appData->path))
        return;
    getcwd(appData->pathWay1, sizeof(appData->pathWay1));
    gchar *gname = NULL;
    gtk_tree_model_get(model, &iter, 1, &gname, -1);
    snprintf(appData->fName1, sizeof(appData->fName1), "%s", gname);
    gtk_widget_set_opacity(appData->pasteEvent, 1);
}