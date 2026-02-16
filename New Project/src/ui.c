#include "ui.h"

/*
 * Callback for activating a folder or file when a row in the GtkTreeView is double-clicked.
 * - If a drive is clicked at root level, navigates into the drive.
 * - If a folder is clicked, navigates into that folder and updates back/forward navigation stacks.
 * - If a file is clicked, opens it using the default system application.
 *
 * Parameters:
 *   treeview - the GtkTreeView containing files and folders
 *   path     - the GtkTreePath corresponding to the activated row
 *   col      - the column that was activated (unused)
 *   data     - pointer to Data structure containing app state and navigation info
 */

void select_directory(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
{
    Data *appData = (Data *)data;
    gtk_widget_set_opacity(appData->nextEvent, 0.1);
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gchar *name;
        if (appData->depthLevel == 0)
            gtk_tree_model_get(model, &iter, 1, &name, -1);
        else
            gtk_tree_model_get(model, &iter, 2, &name, -1);
        appData->forwardCount = 0;
        if (appData->depthLevel == 0)
        {
            gchar driveName[MAX_PATH + 1] = "";
            driveName[0] = name[strlen(name) - 3];
            driveName[1] = ':';
            driveName[2] = '\\';
            driveName[3] = '\\';
            driveName[4] = '\0';
            chdir(driveName);
            appData->depthLevel++;
            open_directory(appData);
        }
        else
        {
            struct stat fstat;
            if (stat(name, &fstat) == 0)
            {
                if (S_ISDIR(fstat.st_mode))
                {
                    appData->depthLevel++;
                    appData->newNode2 = (BackPath *)malloc(sizeof(BackPath));
                    char currentPath[MAX_PATH] = "";
                    getcwd(currentPath, sizeof(currentPath));
                    strcpy(appData->newNode2->path, currentPath);
                    appData->newNode2->next = NULL;
                    if (appData->head2 == NULL)
                    {
                        appData->head2 = appData->newNode2;
                    }
                    else
                    {
                        appData->newNode2->next = appData->head2;
                        appData->head2 = appData->newNode2;
                    }
                    chdir(name);
                    open_directory(appData);
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

/*
 * Callback for right mouse button click on a treeview row.
 * - Displays a context menu for Rename, Copy, Cut, and Delete operations.
 * - Only active when depthLevel > 0 (not at drive selection).
 *
 * Parameters:
 *   treeview - the GtkTreeView where the click occurred
 *   event    - the GdkEventButton containing mouse click info
 *   data     - pointer to Data structure containing app state and tree selection
 *
 * Returns:
 *   TRUE if a menu was displayed, FALSE otherwise
 */

gboolean right_button_click(GtkTreeView *treeview, GdkEventButton *event, gpointer data)
{
    Data *appData = (Data *)data;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3 && appData->depthLevel > 0)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
        // GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(treeview, (gint)event->x, (gint)event->y, &(appData->path), NULL, NULL, NULL))
        {
            if (gtk_tree_selection_path_is_selected(selection, appData->path))
            {
                GtkWidget *menu = gtk_menu_new();
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

                g_signal_connect(item1, "activate", G_CALLBACK(folder_rename), appData);
                g_signal_connect(item2, "activate", G_CALLBACK(copy), appData);
                g_signal_connect(item4, "activate", G_CALLBACK(cut), appData);
                g_signal_connect(item3, "activate", G_CALLBACK(delete), appData);
            }
            return TRUE;
        }
    }
    else
        return FALSE;
}

/*
 * Initializes the main UI of the file manager.
 * - Sets up the main window, buttons, icons, search box, treeview, and scrollable area.
 * - Loads all required icons and scales them appropriately.
 * - Connects signals for row activation, button clicks, and search functionality.
 * - Populates the initial view with drives and disables/enables navigation buttons as needed.
 *
 * Parameters:
 *   data - pointer to Data structure where all UI widgets and state variables are stored
 */

void set_ui(Data *data)
{
    Data *appData = data;
    GtkTreeIter iter;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    appData->labelPath = gtk_label_new(" ");
    appData->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(appData->window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(appData->window), 700, 600);
    gtk_window_set_position(GTK_WINDOW(appData->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(appData->window), 10);

    g_signal_connect(appData->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    appData->boxSearch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    appData->searchEntry = gtk_entry_new();
    appData->searchButton = gtk_button_new_with_label("Search");
    gtk_box_pack_start(GTK_BOX(appData->boxSearch), appData->searchEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(appData->boxSearch), appData->searchButton, FALSE, FALSE, 0);


    appData->backEvent = gtk_event_box_new();
    appData->nextEvent = gtk_event_box_new();
    appData->newFolderEvent = gtk_event_box_new();
    appData->pasteEvent = gtk_event_box_new();
    GError *error = NULL;
    appData->iconPixbuf = gdk_pixbuf_new_from_file("icons/File.png", &error);
    appData->pixbuf1 = gdk_pixbuf_new_from_file("icons/back.png", &error);
    appData->pixbuf2 = gdk_pixbuf_new_from_file("icons/next.png", &error);
    GdkPixbuf *pixbuf3 = gdk_pixbuf_new_from_file("icons/newFolder.png", &error);
    GdkPixbuf *pixbuf4 = gdk_pixbuf_new_from_file("icons/drive.png", &error);
    GdkPixbuf *pixbuf5 = gdk_pixbuf_new_from_file("icons/Paste.png", &error);
    GdkPixbuf *scaledPixbuf1 = gdk_pixbuf_scale_simple(appData->pixbuf1, 40, 40, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaledPixbuf2 = gdk_pixbuf_scale_simple(appData->pixbuf2, 40, 40, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaledPixbuf3 = gdk_pixbuf_scale_simple(pixbuf3, 40, 40, GDK_INTERP_BILINEAR);
    appData->scaledPixbuf4 = gdk_pixbuf_scale_simple(pixbuf4, 40, 40, GDK_INTERP_BILINEAR);
    GdkPixbuf *scaledPixbuf5 = gdk_pixbuf_scale_simple(pixbuf5, 30, 30, GDK_INTERP_BILINEAR);
    appData->image1 = gtk_image_new_from_pixbuf(scaledPixbuf1);
    appData->image2 = gtk_image_new_from_pixbuf(scaledPixbuf2);
    GtkWidget *image3 = gtk_image_new_from_pixbuf(scaledPixbuf3);
    GtkWidget *image4 = gtk_image_new_from_pixbuf(scaledPixbuf5);
    gtk_container_add(GTK_CONTAINER(appData->backEvent), appData->image1);
    gtk_container_add(GTK_CONTAINER(appData->nextEvent), appData->image2);
    gtk_container_add(GTK_CONTAINER(appData->newFolderEvent), image3);
    gtk_container_add(GTK_CONTAINER(appData->pasteEvent), image4);

    appData->parentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    appData->fullBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 250);
    appData->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    appData->boxPath = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    gtk_box_pack_start(GTK_BOX(appData->box), appData->backEvent, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appData->box), appData->nextEvent, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appData->box), appData->newFolderEvent, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appData->box), appData->pasteEvent, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appData->parentBox), appData->fullBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appData->boxPath), appData->labelPath, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appData->parentBox), appData->boxPath, FALSE, FALSE, 0);

    appData->store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

    appData->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(appData->store));
    GtkCellRenderer *pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *icon_column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(icon_column, pixbuf_renderer, TRUE);
    gtk_tree_view_column_add_attribute(icon_column, pixbuf_renderer, "pixbuf", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(appData->treeview), icon_column);
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL);
    PangoFontDescription *font_desc = pango_font_description_new();
    pango_font_description_set_size(font_desc, 14 * PANGO_SCALE);
    g_object_set(renderer, "font-desc", font_desc, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(appData->treeview), col);
    appData->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(appData->scrolledWindow), 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(appData->scrolledWindow),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(appData->scrolledWindow), appData->treeview);

    gtk_box_pack_start(GTK_BOX(appData->parentBox), appData->scrolledWindow, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(appData->fullBox), appData->box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appData->fullBox), appData->boxSearch, FALSE, FALSE, 0);

    g_signal_connect(appData->treeview, "row-activated", G_CALLBACK(select_directory), appData);
    g_signal_connect(appData->backEvent, "button-press-event", G_CALLBACK(go_to_parent_folder), appData);
    g_signal_connect(appData->nextEvent, "button-press-event", G_CALLBACK(go_to_previous_folder), appData);
    g_signal_connect(appData->treeview, "button-press-event", G_CALLBACK(right_button_click), appData);
    g_signal_connect(appData->newFolderEvent, "button-press-event", G_CALLBACK(create_new_folder), appData);
    g_signal_connect(appData->pasteEvent, "button-press-event", G_CALLBACK(copy_function), appData);
    g_signal_connect(appData->searchButton, "clicked", G_CALLBACK(on_search_clicked), appData);

    appData->labelStatus = gtk_label_new("This folder is empty");
    show_drives(appData);
    gtk_widget_set_opacity(appData->nextEvent, 0.1);
    gtk_container_add(GTK_CONTAINER(appData->window), appData->parentBox);
    gtk_widget_show_all(appData->window);
    gtk_widget_hide(appData->boxSearch);
    gtk_main();
}