/**
 * @file nvign.c
 * @brief Implementations for directory navigation: show drives, open directory,
 *        go up (parent), and go forward (previous).
 */

#include "nvign.h"
#include "icon.h"

#include <gtk/gtk.h>
#include <windows.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

/* ── Show drives ────────────────────────────────────────────────────────── */

/**
 * @brief Populates the file list with the available logical drives on the system.
 *
 * @param data Pointer to the Data struct holding all UI and navigation state.
 */
void show_drives(Data *data)
{
    gtk_widget_set_opacity(data->event1, 0.1);
    gtk_widget_set_opacity(data->event3, 0.1);
    gtk_widget_set_opacity(data->event4, 0.1);

    if (gtk_widget_get_parent(data->box_search))
        gtk_widget_hide(data->box_search);

    gtk_label_set_text(GTK_LABEL(data->label_path), "Devices and Drives");
    gtk_widget_set_name(data->label_path, "labelPath");

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        provider,
        "#labelPath { font-size: 20px; font-family: Arial; font-weight: bold; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkTreeIter iter;
    DWORD       drives = GetLogicalDrives();
    gchar       drive;

    for (drive = 'A'; drive <= 'Z'; drive++)
    {
        if (drives & (1 << (drive - 'A')))
        {
            gchar path[MAX_PATH + 1]      = "";
            gchar name1[MAX_PATH + 1]     = "";
            gchar name2[MAX_PATH + 1]     = "";
            gchar real_name[MAX_PATH + 1] = "";

            path[0]  = drive;
            name2[0] = drive;
            strcat(path, ":\\");
            strcat(name2, ":");
            GetVolumeInformation(path, name1, sizeof(name1),
                                 NULL, NULL, NULL, NULL, 0);
            if (name1[0] != '\0')
                strcat(name1, " ");

            sprintf(real_name, "%s(%s)", name1, name2);
            gtk_list_store_append(data->store, &iter);
            gtk_list_store_set(data->store, &iter,
                               0, data->scaled_pixbuf4,
                               1, real_name,
                               -1);
        }
    }
}

/* ── Open directory ─────────────────────────────────────────────────────── */

/**
 * @brief Populates the file list with the contents of the current working directory.
 *
 * @param data Pointer to the Data struct holding all UI and navigation state.
 */
void open_directory(Data *data)
{
    gtk_box_pack_start(GTK_BOX(data->full_box), data->box_search,
                       FALSE, FALSE, 0);

    if (data->is_copy == 1 || data->is_cut == 1)
        gtk_widget_set_opacity(data->event4, 1);

    if (data->nav_forward == 0)
        gtk_widget_set_opacity(data->event2, 0.1);

    gtk_widget_set_opacity(data->event1, 1);
    gtk_widget_set_opacity(data->event3, 1);

    GtkTreeIter iter;
    gtk_list_store_clear(data->store);

    DIR   *dp = opendir(".");
    gchar  path[MAX_PATH + 1];
    getcwd(path, sizeof(path));

    gchar real_path[MAX_PATH + 50];
    sprintf(real_path, "Current path: %s", path);
    gtk_label_set_text(GTK_LABEL(data->label_path), real_path);
    gtk_widget_show_all(data->parent_box);

    int            h = 0;
    struct dirent *entry;

    while ((entry = readdir(dp)) != NULL)
    {
        DWORD att = GetFileAttributes(entry->d_name);
        if (att == INVALID_FILE_ATTRIBUTES)
            continue;
        if (att & FILE_ATTRIBUTE_HIDDEN || att & FILE_ATTRIBUTE_SYSTEM)
            continue;
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0)
        {
            h++;
            char full_path[MAX_PATH] = "";
            snprintf(full_path, sizeof(full_path), "%s\\%s",
                     path, entry->d_name);

            struct stat  file_stat;
            stat(full_path, &file_stat);
            GdkPixbuf   *scaled_pixbuf;

            if (S_ISDIR(file_stat.st_mode))
            {
                scaled_pixbuf = gdk_pixbuf_scale_simple(
                    data->icon_pixbuf, 48, 48, GDK_INTERP_BILINEAR);
            }
            else
            {
                scaled_pixbuf = get_file_icon(entry->d_name, 48);
                if (!scaled_pixbuf)
                    scaled_pixbuf = gtk_icon_theme_load_icon(
                        gtk_icon_theme_get_default(),
                        "text-x-generic", 48, 0, NULL);
            }

            gtk_list_store_append(data->store, &iter);
            gtk_list_store_set(data->store, &iter,
                               0, scaled_pixbuf,
                               1, entry->d_name,
                               2, full_path,
                               -1);
            if (scaled_pixbuf)
                g_object_unref(scaled_pixbuf);
        }
    }

    GtkWidget *child = gtk_bin_get_child(GTK_BIN(data->scrolled_window));
    if (child)
        gtk_container_remove(GTK_CONTAINER(data->scrolled_window), child);

    if (h == 0)
    {
        GtkWidget *empty_label = gtk_label_new("This folder is empty");
        gtk_container_add(GTK_CONTAINER(data->scrolled_window), empty_label);
    }
    else
    {
        gtk_container_add(GTK_CONTAINER(data->scrolled_window), data->treeview);
    }

    gtk_widget_show_all(data->scrolled_window);
    closedir(dp);
}

/* ── Navigate up (parent folder) ────────────────────────────────────────── */

/**
 * @brief GTK signal handler that navigates one level up in the directory hierarchy.
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void go_to_parent_folder(GtkWidget *widget, GdkEventButton *event,
                         gpointer data)
{
    Data *app_data = (Data *)data;
    gtk_widget_set_opacity(app_data->event2, 1);

    if (app_data->depth != 0)
    {
        app_data->depth--;
        app_data->nav_forward++;

        gchar current_path[MAX_PATH + 1] = "";
        getcwd(current_path, sizeof(current_path));

        /* Push the current directory onto the forward stack. */
        app_data->new_node_next = (NextPath *)malloc(sizeof(NextPath));
        strcpy(app_data->new_node_next->path, current_path);
        app_data->new_node_next->next = NULL;

        if (app_data->head_next == NULL)
            app_data->head_next = app_data->new_node_next;
        else
        {
            app_data->new_node_next->next = app_data->head_next;
            app_data->head_next = app_data->new_node_next;
        }

        if (app_data->depth == 0)
        {
            gtk_list_store_clear(app_data->store);
            show_drives(app_data);
        }
        else
        {
            chdir(app_data->head_back->path);
            BackPath *dd        = app_data->head_back;
            app_data->head_back = app_data->head_back->next;
            free(dd);
            open_directory(app_data);
        }
    }
}

/* ── Navigate forward (next folder) ────────────────────────────────────── */

/**
 * @brief GTK signal handler that navigates forward to a previously visited directory.
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void go_to_previous_folder(GtkWidget *widget, GdkEventButton *event,
                            gpointer data)
{
    Data *app_data = (Data *)data;

    if (app_data->nav_forward != 0)
    {
        app_data->nav_forward--;
        app_data->depth++;

        /* Push the current directory onto the backward stack. */
        app_data->new_node_back = (BackPath *)malloc(sizeof(BackPath));
        char current_path[MAX_PATH] = "";
        getcwd(current_path, sizeof(current_path));
        strcpy(app_data->new_node_back->path, current_path);
        app_data->new_node_back->next = NULL;

        if (app_data->head_back == NULL)
            app_data->head_back = app_data->new_node_back;
        else
        {
            app_data->new_node_back->next = app_data->head_back;
            app_data->head_back = app_data->new_node_back;
        }

        char folder_name[MAX_PATH + 1];
        strcpy(folder_name, app_data->head_next->path);
        NextPath *dd        = app_data->head_next;
        app_data->head_next = app_data->head_next->next;
        free(dd);
        chdir(folder_name);
        open_directory(app_data);
    }
}
