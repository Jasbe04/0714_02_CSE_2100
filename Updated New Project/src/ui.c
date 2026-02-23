/**
 * @file ui.c
 * @brief Implementations for UI construction, directory row activation,
 *        and the right-click context menu.
 */

#include "ui.h"
#include "nvign.h"
#include "menu_op.h"
#include "file_op.h"
#include "search.h"

#include <gtk/gtk.h>
#include <windows.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* ── Select directory / open file ───────────────────────────────────────── */

/**
 * @brief GTK signal handler triggered when the user double-clicks a row.
 *
 * @param treeview The GtkTreeView that received the row-activated signal.
 * @param path     Tree path of the activated row.
 * @param col      The activated column (unused).
 * @param data     Pointer to the Data struct cast to gpointer.
 */
void select_directory(GtkTreeView *treeview, GtkTreePath *path,
                      GtkTreeViewColumn *col, gpointer data)
{
    Data         *app_data = (Data *)data;
    gtk_widget_set_opacity(app_data->event2, 0.1);

    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkTreeIter   iter;

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gchar *name;
        if (app_data->depth == 0)
            gtk_tree_model_get(model, &iter, 1, &name, -1);
        else
            gtk_tree_model_get(model, &iter, 2, &name, -1);

        app_data->nav_forward = 0;

        if (app_data->depth == 0)
        {
            /* Extract the drive letter from the display string, e.g. "Local Disk (C)". */
            gchar drive_name[MAX_PATH + 1] = "";
            drive_name[0] = name[strlen(name) - 3];
            drive_name[1] = ':';
            drive_name[2] = '\\';
            drive_name[3] = '\\';
            drive_name[4] = '\0';
            chdir(drive_name);
            app_data->depth++;
            open_directory(app_data);
        }
        else
        {
            struct stat file_stat;
            if (stat(name, &file_stat) == 0)
            {
                if (S_ISDIR(file_stat.st_mode))
                {
                    app_data->depth++;

                    /* Push the current directory onto the backward stack. */
                    app_data->new_node_back =
                        (BackPath *)malloc(sizeof(BackPath));
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
                    chdir(name);
                    open_directory(app_data);
                }
                else
                {
                    ShellExecute(NULL, "open", name, NULL, NULL, SW_SHOWNORMAL);
                }
            }
        }
        g_free(name);
    }
}

/* ── Right-click context menu ───────────────────────────────────────────── */

/**
 * @brief GTK signal handler for mouse button presses on the tree view.
 *
 * @param treeview The GtkTreeView that received the event.
 * @param event    The GDK button event.
 * @param data     Pointer to the Data struct cast to gpointer.
 * @return gboolean TRUE if a context menu was shown, FALSE otherwise.
 */
gboolean right_button_click(GtkTreeView *treeview, GdkEventButton *event,
                             gpointer data)
{
    Data *app_data = (Data *)data;

    if (event->type == GDK_BUTTON_PRESS &&
        event->button == 3 &&
        app_data->depth > 0)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
        GtkTreePath      *path;

        if (gtk_tree_view_get_path_at_pos(treeview,
                                          (gint)event->x, (gint)event->y,
                                          &path, NULL, NULL, NULL))
        {
            if (gtk_tree_selection_path_is_selected(selection, path))
            {
                /* Bundle state + path into a two-element array for callbacks. */
                static gpointer cb_data[2];
                cb_data[0] = app_data;
                cb_data[1] = path;

                GtkWidget *menu  = gtk_menu_new();
                GtkWidget *item1 = gtk_menu_item_new_with_label("Rename");
                GtkWidget *item2 = gtk_menu_item_new_with_label("Copy");
                GtkWidget *item3 = gtk_menu_item_new_with_label("Delete");
                GtkWidget *item4 = gtk_menu_item_new_with_label("Cut");

                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item1);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item2);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item4);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item3);
                gtk_widget_show_all(menu);
                gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

                g_signal_connect(item1, "activate",
                                 G_CALLBACK(folder_rename), cb_data);
                g_signal_connect(item2, "activate",
                                 G_CALLBACK(copy),          cb_data);
                g_signal_connect(item4, "activate",
                                 G_CALLBACK(cut),           cb_data);
                g_signal_connect(item3, "activate",
                                 G_CALLBACK(delete),        cb_data);
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* ── UI construction ────────────────────────────────────────────────────── */

/**
 * @brief Builds and displays the complete file-manager UI, then starts the
 *        GTK main event loop.
 *
 * @param data Pointer to the heap-allocated Data struct to populate and use.
 */
void set_ui(Data *data)
{
    GtkWidget          *label1;
    GtkWidget          *label2;
    GtkWidget          *label3;
    GtkTreeIter         iter;
    GtkTreeViewColumn  *col;
    GtkCellRenderer    *renderer;

    /* ── Main window ────────────────────────────────────────────────────── */
    data->label_path = gtk_label_new(" ");
    data->window     = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(data->window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(data->window), 700, 600);
    gtk_window_set_position(GTK_WINDOW(data->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(data->window), 10);
    g_signal_connect(data->window, "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);

    /* ── Search bar ─────────────────────────────────────────────────────── */
    data->box_search    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    data->search_entry  = gtk_entry_new();
    data->search_button = gtk_button_new_with_label("Search");
    gtk_box_pack_start(GTK_BOX(data->box_search),
                       data->search_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(data->box_search),
                       data->search_button, FALSE, FALSE, 0);

    /* ── Toolbar labels (Back / Next / New Folder) ──────────────────────── */
    label1 = gtk_label_new("Back");
    label2 = gtk_label_new("Next");
    label3 = gtk_label_new("New Folder");

    /* ── Event boxes ────────────────────────────────────────────────────── */
    data->event1 = gtk_event_box_new();
    data->event2 = gtk_event_box_new();
    data->event3 = gtk_event_box_new();
    data->event4 = gtk_event_box_new();

    /* ── Load and scale toolbar pixbufs ─────────────────────────────────── */
    GError    *error = NULL;
    data->icon_pixbuf  = gdk_pixbuf_new_from_file("File.png",      &error);
    data->pixbuf1      = gdk_pixbuf_new_from_file("back.png",       &error);
    data->pixbuf2      = gdk_pixbuf_new_from_file("next.png",       &error);
    GdkPixbuf *pixbuf3 = gdk_pixbuf_new_from_file("newFolder.png",  &error);
    GdkPixbuf *pixbuf4 = gdk_pixbuf_new_from_file("drive.png",      &error);
    GdkPixbuf *pixbuf5 = gdk_pixbuf_new_from_file("Paste.png",      &error);

    GdkPixbuf *scaled_pixbuf1 =
        gdk_pixbuf_scale_simple(data->pixbuf1, 40, 40, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaled_pixbuf2 =
        gdk_pixbuf_scale_simple(data->pixbuf2, 40, 40, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaled_pixbuf3 =
        gdk_pixbuf_scale_simple(pixbuf3, 40, 40, GDK_INTERP_BILINEAR);
    data->scaled_pixbuf4 =
        gdk_pixbuf_scale_simple(pixbuf4, 40, 40, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaled_pixbuf5 =
        gdk_pixbuf_scale_simple(pixbuf5, 30, 30, GDK_INTERP_BILINEAR);

    data->image1     = gtk_image_new_from_pixbuf(scaled_pixbuf1);
    data->image2     = gtk_image_new_from_pixbuf(scaled_pixbuf2);
    GtkWidget *image3 = gtk_image_new_from_pixbuf(scaled_pixbuf3);
    GtkWidget *image4 = gtk_image_new_from_pixbuf(scaled_pixbuf5);

    gtk_container_add(GTK_CONTAINER(data->event1), data->image1);
    gtk_container_add(GTK_CONTAINER(data->event2), data->image2);
    gtk_container_add(GTK_CONTAINER(data->event3), image3);
    gtk_container_add(GTK_CONTAINER(data->event4), image4);

    /* ── Layout boxes ───────────────────────────────────────────────────── */
    data->parent_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,   10);
    data->full_box   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 250);
    data->box        = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,  10);
    data->box_path   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,  10);

    gtk_box_pack_start(GTK_BOX(data->box), data->event1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->box), data->event2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->box), data->event3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->box), data->event4, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->full_box),   data->box,        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->parent_box), data->full_box,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->box_path),   data->label_path, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->parent_box), data->box_path,   FALSE, FALSE, 0);

    /* ── Tree view ──────────────────────────────────────────────────────── */
    data->store    = gtk_list_store_new(3, GDK_TYPE_PIXBUF,
                                        G_TYPE_STRING, G_TYPE_STRING);
    data->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(data->store));

    GtkCellRenderer   *pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *icon_column     = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(icon_column, pixbuf_renderer, TRUE);
    gtk_tree_view_column_add_attribute(icon_column, pixbuf_renderer,
                                       "pixbuf", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->treeview), icon_column);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Name", renderer,
                                                    "text", 1, NULL);
    PangoFontDescription *font_desc = pango_font_description_new();
    pango_font_description_set_size(font_desc, 14 * PANGO_SCALE);
    g_object_set(renderer, "font-desc", font_desc, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(data->treeview), col);

    /* ── Scrolled window ────────────────────────────────────────────────── */
    data->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(data->scrolled_window), 5);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(data->scrolled_window),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(data->scrolled_window), data->treeview);
    gtk_box_pack_start(GTK_BOX(data->parent_box), data->scrolled_window,
                       TRUE, TRUE, 0);

    /* ── Signal connections ─────────────────────────────────────────────── */
    g_signal_connect(data->treeview,    "row-activated",
                     G_CALLBACK(select_directory),   data);
    g_signal_connect(data->event1,      "button-press-event",
                     G_CALLBACK(go_to_parent_folder),   data);
    g_signal_connect(data->event2,      "button-press-event",
                     G_CALLBACK(go_to_previous_folder), data);
    g_signal_connect(data->treeview,    "button-press-event",
                     G_CALLBACK(right_button_click),    data);
    g_signal_connect(data->event3,      "button-press-event",
                     G_CALLBACK(create_new_folder),     data);
    g_signal_connect(data->event4,      "button-press-event",
                     G_CALLBACK(copy_function),         data);
    g_signal_connect(data->search_button, "clicked",
                     G_CALLBACK(on_search_clicked),     data);

    /* ── Initial display ────────────────────────────────────────────────── */
    data->label_status = gtk_label_new("This folder is empty");
    show_drives(data);
    gtk_widget_set_opacity(data->event2, 0.1);
    gtk_container_add(GTK_CONTAINER(data->window), data->parent_box);
    gtk_widget_show_all(data->window);
    gtk_main();
}
