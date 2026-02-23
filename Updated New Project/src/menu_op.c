/**
 * @file menu_op.c
 * @brief Implementations for right-click context-menu operations:
 *        rename, delete, copy, and cut.
 */

#include "menu_op.h"
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

/* ── Rename ─────────────────────────────────────────────────────────────── */

/**
 * @brief GTK signal handler that prompts the user to rename the selected item.
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void folder_rename(GtkMenuItem *item, gpointer data)
{
    Data         *app_data = (Data *)((gpointer *)data)[0];
    GtkTreePath  *path     = (GtkTreePath *)((gpointer *)data)[1];

    GtkWidget    *dialog;
    GtkWidget    *entry;
    GtkWidget    *area;
    GtkTreeIter   iter;
    GtkTreeModel *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(app_data->treeview));

    gtk_tree_model_get_iter(model, &iter, path);
    gchar *folder_name = NULL;
    gtk_tree_model_get(model, &iter, 1, &folder_name, -1);

    dialog = gtk_dialog_new_with_buttons(
        "Rename", GTK_WINDOW(app_data->window), GTK_DIALOG_MODAL,
        "Ok", GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
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
                if (rename(folder_name, name) != 0)
                {
                    gchar     *error_message = strerror(errno);
                    GtkWidget *alert = gtk_message_dialog_new(
                        GTK_WINDOW(app_data->window), GTK_DIALOG_MODAL,
                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error_message);
                    gtk_dialog_run(GTK_DIALOG(alert));
                    g_free(error_message);
                    gtk_widget_destroy(alert);
                }
                else
                {
                    open_directory(app_data);
                    break;
                }
            }
            else
                break;
        }
        else
            break;
    }

    g_free(folder_name);
    gtk_widget_destroy(dialog);
}

/* ── Delete helper (recursive) ──────────────────────────────────────────── */

/**
 * @brief Recursively deletes a file or directory, updating the progress bar
 *        after each individual file is removed.
 *
 * @param path     Absolute path to the file or directory to delete.
 * @param progress Pointer to the CopyProgress struct used to track progress.
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
        gchar          full_path[MAX_PATH + 1];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(full_path, sizeof(full_path), "%s\\%s",
                     path, entry->d_name);
            delete_function(full_path, progress);
        }
        closedir(dp);

        if (rmdir(path))
        {
            gchar msg[256];
            snprintf(msg, sizeof(msg),
                     "Failed to remove directory: %s", path);
            MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
        }
    }
    else
    {
        FILE     *f         = fopen(path, "rb");
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
            progress->copied_bytes += file_size;
            update_progress_bar(progress);
        }
    }
}

/* ── Delete (menu action) ───────────────────────────────────────────────── */

/**
 * @brief GTK signal handler that confirms with the user and then deletes the
 *        selected file or folder.
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void delete(GtkMenuItem *item, gpointer data)
{
    Data         *app_data = (Data *)((gpointer *)data)[0];
    GtkTreePath  *path     = (GtkTreePath *)((gpointer *)data)[1];

    GtkTreeIter   iter;
    GtkTreeModel *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(app_data->treeview));

    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;

    gchar *fname = NULL;
    gtk_tree_model_get(model, &iter, 1, &fname, -1);
    if (!fname)
        return;

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(app_data->window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Are you sure you want to delete '%s'?", fname);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response != GTK_RESPONSE_YES)
    {
        g_free(fname);
        return;
    }

    GtkWidget *progress_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(progress_window), "Deleting...");
    gtk_window_set_default_size(GTK_WINDOW(progress_window), 400, 60);
    gtk_window_set_position(GTK_WINDOW(progress_window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(progress_window), 10);
    gtk_window_set_resizable(GTK_WINDOW(progress_window), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(progress_window),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    GtkWidget *progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
    gtk_container_add(GTK_CONTAINER(progress_window), progress_bar);
    gtk_widget_show_all(progress_window);

    CopyProgress prog = {0};
    prog.progress_bar = progress_bar;
    prog.total_bytes  = calculate_total_size(fname);
    prog.copied_bytes = 0;
    prog.fraction     = 0.0;

    char full_path[MAX_PATH];
    getcwd(full_path, sizeof(full_path));
    strncat(full_path, "\\", sizeof(full_path) - strlen(full_path) - 1);
    strncat(full_path, fname, sizeof(full_path) - strlen(full_path) - 1);

    delete_function(full_path, &prog);
    update_progress_bar(&prog);

    gtk_widget_destroy(progress_window);
    open_directory(app_data);
    g_free(fname);
}

/* ── Copy (menu action) ─────────────────────────────────────────────────── */

/**
 * @brief GTK signal handler that marks the selected item for copying.
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void copy(GtkMenuItem *item, gpointer data)
{
    Data         *app_data = (Data *)((gpointer *)data)[0];
    GtkTreePath  *path     = (GtkTreePath *)((gpointer *)data)[1];

    app_data->is_copy = 1;
    app_data->is_cut  = 0;

    GtkTreeIter   iter;
    GtkTreeModel *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(app_data->treeview));

    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;

    getcwd(app_data->pathway1, sizeof(app_data->pathway1));
    gchar *gname = NULL;
    gtk_tree_model_get(model, &iter, 1, &gname, -1);
    snprintf(app_data->fname1, sizeof(app_data->fname1), "%s", gname);
    gtk_widget_set_opacity(app_data->event4, 1);
}

/* ── Cut (menu action) ──────────────────────────────────────────────────── */

/**
 * @brief GTK signal handler that marks the selected item for cutting (move).
 *
 * @param item The menu item that was activated (unused).
 * @param data Pointer to a gpointer[2] array: [0] Data*, [1] GtkTreePath*.
 */
void cut(GtkMenuItem *item, gpointer data)
{
    Data         *app_data = (Data *)((gpointer *)data)[0];
    GtkTreePath  *path     = (GtkTreePath *)((gpointer *)data)[1];

    app_data->is_cut  = 1;
    app_data->is_copy = 0;

    GtkTreeIter   iter;
    GtkTreeModel *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(app_data->treeview));

    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;

    getcwd(app_data->pathway1, sizeof(app_data->pathway1));
    gchar *gname = NULL;
    gtk_tree_model_get(model, &iter, 1, &gname, -1);
    snprintf(app_data->fname1, sizeof(app_data->fname1), "%s", gname);
    gtk_widget_set_opacity(app_data->event4, 1);
}
