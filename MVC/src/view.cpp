/**
 * @file view.cpp
 * @brief MVC View implementation.
 *
 * The View contains ALL GTK widget construction and rendering.  It has no
 * knowledge of the filesystem; it only knows how to display FileEntry and
 * ClipboardState data pushed to it by the Model via IModelObserver callbacks.
 *
 * User-input signals are forwarded to the Controller; the View itself never
 * calls Model methods.
 */

#include "view.hpp"
#include "controller.hpp"

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>

// ── Construction / destruction ────────────────────────────────────────────────

FileManagerView::FileManagerView() {}

FileManagerView::~FileManagerView()
{
    if (m_iconPixbuf)
        g_object_unref(m_iconPixbuf);
    if (m_scaledDriveIcon)
        g_object_unref(m_scaledDriveIcon);
    if (m_pixbufBack)
        g_object_unref(m_pixbufBack);
    if (m_pixbufNext)
        g_object_unref(m_pixbufNext);
}

void FileManagerView::setController(FileManagerController *ctrl)
{
    m_ctrl = ctrl;
}

// ── IModelObserver ────────────────────────────────────────────────────────────

void FileManagerView::onDirectoryChanged(const std::vector<FileEntry> &entries,
                                         const std::string &currentPath)
{
    m_isDriveView = false;

    setPathLabel("Current path: " + currentPath);
    setBackEnabled(true);
    setNewFolderEnabled(true);

    // Show search bar.
    if (m_boxSearch)
        gtk_widget_show(m_boxSearch);

    populateStore(entries);
}

void FileManagerView::onDrivesChanged(const std::vector<FileEntry> &drives)
{
    m_isDriveView = true;

    // Hide search bar and toolbar buttons that only apply inside a directory.
    if (m_boxSearch)
        gtk_widget_hide(m_boxSearch);
    setBackEnabled(false);
    setForwardEnabled(false);
    setPasteEnabled(false);
    setNewFolderEnabled(false);

    setPathLabel("Devices and Drives");

    gtk_list_store_clear(m_store);

    GtkTreeIter iter;
    for (const FileEntry &fe : drives)
    {
        gtk_list_store_append(m_store, &iter);
        gtk_list_store_set(m_store, &iter,
                           0, m_scaledDriveIcon,
                           1, fe.name.c_str(),
                           2, fe.fullPath.c_str(),
                           -1);
    }

    // Swap scrolled-window content.
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(m_scrolledWindow));
    if (child)
        gtk_container_remove(GTK_CONTAINER(m_scrolledWindow), child);
    gtk_container_add(GTK_CONTAINER(m_scrolledWindow), m_treeview);
    gtk_widget_show_all(m_scrolledWindow);
}

void FileManagerView::onClipboardChanged(const ClipboardState &state)
{
    setPasteEnabled(state.hasPending());
}

void FileManagerView::onSearchResults(const std::vector<FileEntry> &results)
{
    if (m_boxSearch)
        gtk_widget_show(m_boxSearch);

    populateStore(results);
    setPathLabel("Search results (" + std::to_string(results.size()) + " items)");
}

// ── Build / run ───────────────────────────────────────────────────────────────

void FileManagerView::build()
{
    // ── Main window ───────────────────────────────────────────────────────────
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(m_window), 700, 600);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(m_window), 10);
    g_signal_connect(m_window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    // ── Search bar ────────────────────────────────────────────────────────────
    m_boxSearch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    m_searchEntry = gtk_entry_new();
    m_searchButton = gtk_button_new_with_label("Search");
    gtk_box_pack_start(GTK_BOX(m_boxSearch), m_searchEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(m_boxSearch), m_searchButton, FALSE, FALSE, 0);

    m_labelPath = gtk_label_new(" ");
    m_labelStatus = gtk_label_new("This folder is empty");

    loadPixbufs();
    buildToolbar();
    buildTreeView();
    buildLayout();
    connectSignals();

    // Forward button starts disabled; search bar hidden until inside a dir.
    gtk_widget_set_opacity(m_event2, 0.1);
    gtk_container_add(GTK_CONTAINER(m_window), m_parentBox);
}

void FileManagerView::run()
{
    // All widgets exist and the Model has already pushed the initial drive list
    // via onDrivesChanged(), so we can safely show everything now.
    gtk_widget_show_all(m_window);
    gtk_widget_hide(m_boxSearch); // Hidden until the user enters a directory.

    gtk_main();
}

// ── Widget builders ───────────────────────────────────────────────────────────

void FileManagerView::loadPixbufs()
{
    GError *err = nullptr;
    m_iconPixbuf = gdk_pixbuf_new_from_file("icons/File.png", &err);
    m_pixbufBack = gdk_pixbuf_new_from_file("icons/back.png", &err);
    m_pixbufNext = gdk_pixbuf_new_from_file("icons/next.png", &err);

    GdkPixbuf *pbNewFolder = gdk_pixbuf_new_from_file("icons/newFolder.png", &err);
    GdkPixbuf *pbDrive = gdk_pixbuf_new_from_file("icons/drive.png", &err);
    GdkPixbuf *pbPaste = gdk_pixbuf_new_from_file("icons/Paste.png", &err);

    m_scaledDriveIcon =
        gdk_pixbuf_scale_simple(pbDrive, 40, 40, GDK_INTERP_BILINEAR);

    m_image1 = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(m_pixbufBack, 40, 40, GDK_INTERP_BILINEAR));
    m_image2 = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(m_pixbufNext, 40, 40, GDK_INTERP_BILINEAR));

    GtkWidget *imgNewFolder = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(pbNewFolder, 40, 40, GDK_INTERP_BILINEAR));
    GtkWidget *imgPaste = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(pbPaste, 30, 30, GDK_INTERP_BILINEAR));

    m_event1 = gtk_event_box_new();
    m_event2 = gtk_event_box_new();
    m_event3 = gtk_event_box_new();
    m_event4 = gtk_event_box_new();

    gtk_container_add(GTK_CONTAINER(m_event1), m_image1);
    gtk_container_add(GTK_CONTAINER(m_event2), m_image2);
    gtk_container_add(GTK_CONTAINER(m_event3), imgNewFolder);
    gtk_container_add(GTK_CONTAINER(m_event4), imgPaste);

    if (pbNewFolder)
        g_object_unref(pbNewFolder);
    if (pbDrive)
        g_object_unref(pbDrive);
    if (pbPaste)
        g_object_unref(pbPaste);
}

void FileManagerView::buildToolbar()
{
    // Toolbar event-boxes are built in loadPixbufs().
    // Labels kept for future use.
    (void)gtk_label_new("Back");
    (void)gtk_label_new("Next");
    (void)gtk_label_new("New Folder");
}

void FileManagerView::buildTreeView()
{
    m_store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
    m_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_store));

    // Icon column.
    GtkCellRenderer *pixRend = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *iconCol = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(iconCol, pixRend, TRUE);
    gtk_tree_view_column_add_attribute(iconCol, pixRend, "pixbuf", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeview), iconCol);

    // Name column.
    GtkCellRenderer *textRend = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *nameCol =
        gtk_tree_view_column_new_with_attributes("Name", textRend, "text", 1, nullptr);
    PangoFontDescription *fontDesc = pango_font_description_new();
    pango_font_description_set_size(fontDesc, 14 * PANGO_SCALE);
    g_object_set(textRend, "font-desc", fontDesc, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeview), nameCol);
}

void FileManagerView::buildLayout()
{
    m_parentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    m_fullBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 250);
    m_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    m_boxPath = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    gtk_box_pack_start(GTK_BOX(m_box), m_event1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_box), m_event2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_box), m_event3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_box), m_event4, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(m_fullBox), m_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_fullBox), m_boxSearch, TRUE, TRUE, 0); // always in layout, shown/hidden as needed
    gtk_box_pack_start(GTK_BOX(m_parentBox), m_fullBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_boxPath), m_labelPath, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_parentBox), m_boxPath, FALSE, FALSE, 0);

    m_scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_set_border_width(GTK_CONTAINER(m_scrolledWindow), 5);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(m_scrolledWindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(m_parentBox), m_scrolledWindow, TRUE, TRUE, 0);
}

void FileManagerView::connectSignals()
{
    g_signal_connect(m_treeview, "row-activated",
                     G_CALLBACK(onRowActivated), this);
    g_signal_connect(m_treeview, "button-press-event",
                     G_CALLBACK(onButtonPress), this);
    g_signal_connect(m_event1, "button-press-event",
                     G_CALLBACK(onBackClicked), this);
    g_signal_connect(m_event2, "button-press-event",
                     G_CALLBACK(onForwardClicked), this);
    g_signal_connect(m_event3, "button-press-event",
                     G_CALLBACK(onNewFolderClicked), this);
    g_signal_connect(m_event4, "button-press-event",
                     G_CALLBACK(onPasteClicked), this);
    g_signal_connect(m_searchButton, "clicked",
                     G_CALLBACK(onSearchClicked), this);
}

// ── Rendering helpers ─────────────────────────────────────────────────────────

void FileManagerView::populateStore(const std::vector<FileEntry> &entries)
{
    gtk_list_store_clear(m_store);

    GtkTreeIter iter;
    GtkIconTheme *theme = gtk_icon_theme_get_default();

    for (const FileEntry &fe : entries)
    {
        GdkPixbuf *icon = nullptr;

        if (fe.isDir)
        {
            icon = m_iconPixbuf
                       ? gdk_pixbuf_scale_simple(m_iconPixbuf, 48, 48, GDK_INTERP_BILINEAR)
                       : nullptr;
        }
        else
        {
            // Try GIO content-type icon.
            GFile *gf = g_file_new_for_path(fe.fullPath.c_str());
            GFileInfo *info = g_file_query_info(
                gf,
                G_FILE_ATTRIBUTE_STANDARD_ICON,
                G_FILE_QUERY_INFO_NONE, nullptr, nullptr);

            if (info)
            {
                GIcon *gicon = g_file_info_get_icon(info);
                GtkIconInfo *ii = gtk_icon_theme_lookup_by_gicon(
                    theme, gicon, 48, (GtkIconLookupFlags)0);
                if (ii)
                {
                    icon = gtk_icon_info_load_icon(ii, nullptr);
                    g_object_unref(ii);
                }
                g_object_unref(info);
            }
            g_object_unref(gf);

            if (!icon)
                icon = gtk_icon_theme_load_icon(
                    theme, "text-x-generic", 48, (GtkIconLookupFlags)0, nullptr);
        }

        // Scale if needed.
        if (icon &&
            (gdk_pixbuf_get_width(icon) != 48 || gdk_pixbuf_get_height(icon) != 48))
        {
            GdkPixbuf *scaled = gdk_pixbuf_scale_simple(icon, 48, 48, GDK_INTERP_BILINEAR);
            g_object_unref(icon);
            icon = scaled;
        }

        gtk_list_store_append(m_store, &iter);
        gtk_list_store_set(m_store, &iter,
                           0, icon,
                           1, fe.name.c_str(),
                           2, fe.fullPath.c_str(),
                           -1);
        if (icon)
            g_object_unref(icon);
    }

    GtkWidget *child = gtk_bin_get_child(GTK_BIN(m_scrolledWindow));
    if (child)
        gtk_container_remove(GTK_CONTAINER(m_scrolledWindow), child);

    if (entries.empty())
    {
        // Safely destroy old label if exists
        if (m_labelStatus)
        {
            if (GTK_IS_WIDGET(m_labelStatus))
                gtk_widget_destroy(m_labelStatus);
            m_labelStatus = nullptr;
        }
        // Create a fresh label
        m_labelStatus = gtk_label_new("This folder is empty");
        gtk_container_add(GTK_CONTAINER(m_scrolledWindow), m_labelStatus);
        gtk_widget_show(m_labelStatus);
    }
    else
    {
        // Safely destroy old label if exists
        if (m_labelStatus)
        {
            if (GTK_IS_WIDGET(m_labelStatus))
                gtk_widget_destroy(m_labelStatus);
            m_labelStatus = nullptr;
        }
        gtk_container_add(GTK_CONTAINER(m_scrolledWindow), m_treeview);
    }
    gtk_widget_show_all(m_scrolledWindow);
}

void FileManagerView::setPathLabel(const std::string &text)
{
    gtk_label_set_text(GTK_LABEL(m_labelPath), text.c_str());
}

void FileManagerView::setForwardEnabled(bool enabled)
{
    gtk_widget_set_opacity(m_event2, enabled ? 1.0 : 0.1);
}

void FileManagerView::setBackEnabled(bool enabled)
{
    gtk_widget_set_opacity(m_event1, enabled ? 1.0 : 0.1);
}

void FileManagerView::setPasteEnabled(bool enabled)
{
    gtk_widget_set_opacity(m_event4, enabled ? 1.0 : 0.1);
}

void FileManagerView::setNewFolderEnabled(bool enabled)
{
    gtk_widget_set_opacity(m_event3, enabled ? 1.0 : 0.1);
}

// ── Accessors ─────────────────────────────────────────────────────────────────

std::string FileManagerView::selectedName() const
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_treeview));
    GtkTreeModel *model = nullptr;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return "";

    gchar *name = nullptr;
    gtk_tree_model_get(model, &iter, 1, &name, -1);
    std::string result(name ? name : "");
    g_free(name);
    return result;
}

std::string FileManagerView::selectedFullPath() const
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_treeview));
    GtkTreeModel *model = nullptr;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return "";

    gchar *path = nullptr;
    gtk_tree_model_get(model, &iter, 2, &path, -1);
    std::string result(path ? path : "");
    g_free(path);
    return result;
}

std::string FileManagerView::nameAtPath(GtkTreePath *path) const
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(m_treeview));
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path))
        return "";

    gchar *name = nullptr;
    gtk_tree_model_get(model, &iter, 1, &name, -1);
    std::string result(name ? name : "");
    g_free(name);
    return result;
}

// ── Static trampolines ────────────────────────────────────────────────────────

/*static*/
void FileManagerView::onRowActivated(GtkTreeView *,
                                     GtkTreePath *path,
                                     GtkTreeViewColumn *,
                                     gpointer userData)
{
    auto *self = static_cast<FileManagerView *>(userData);
    if (self->m_ctrl)
        self->m_ctrl->onItemActivated(path);
}

/*static*/
gboolean FileManagerView::onButtonPress(GtkTreeView *tv,
                                        GdkEventButton *event,
                                        gpointer userData)
{
    auto *self = static_cast<FileManagerView *>(userData);
    if (event->type == GDK_BUTTON_PRESS && event->button == 3 && !self->m_isDriveView)
    {
        self->showContextMenu(tv, event);
        return TRUE;
    }
    return FALSE;
}

/*static*/
void FileManagerView::onBackClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    auto *self = static_cast<FileManagerView *>(userData);
    if (self->m_ctrl)
        self->m_ctrl->onBack();
}

/*static*/
void FileManagerView::onForwardClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    auto *self = static_cast<FileManagerView *>(userData);
    if (self->m_ctrl)
        self->m_ctrl->onForward();
}

/*static*/
void FileManagerView::onNewFolderClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    auto *self = static_cast<FileManagerView *>(userData);
    if (self->m_ctrl && !self->m_isDriveView)
        self->m_ctrl->onNewFolder();
}

/*static*/
void FileManagerView::onPasteClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    auto *self = static_cast<FileManagerView *>(userData);
    if (self->m_ctrl)
        self->m_ctrl->onPaste();
}

/*static*/
void FileManagerView::onSearchClicked(GtkButton *, gpointer userData)
{
    auto *self = static_cast<FileManagerView *>(userData);
    if (self->m_ctrl)
        self->m_ctrl->onSearch();
}

// ── Context menu ─────────────────────────────────────────────────────────────

void FileManagerView::showContextMenu(GtkTreeView *tv, GdkEventButton *event)
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);
    GtkTreePath *path = nullptr;

    if (!gtk_tree_view_get_path_at_pos(tv,
                                       (gint)event->x, (gint)event->y,
                                       &path, nullptr, nullptr, nullptr))
        return;

    if (!gtk_tree_selection_path_is_selected(sel, path))
    {
        gtk_tree_path_free(path);
        return;
    }

    GtkWidget *menu = gtk_menu_new();
    GtkWidget *iRen = gtk_menu_item_new_with_label("Rename");
    GtkWidget *iCopy = gtk_menu_item_new_with_label("Copy");
    GtkWidget *iCut = gtk_menu_item_new_with_label("Cut");
    GtkWidget *iDel = gtk_menu_item_new_with_label("Delete");

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iRen);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iCopy);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iCut);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iDel);
    gtk_widget_show_all(menu);

    // Store context on each item using g_object_set_data.
    for (GtkWidget *item : {iRen, iCopy, iCut, iDel})
    {
        g_object_set_data(G_OBJECT(item), "view", this);
        g_object_set_data(G_OBJECT(item), "path", gtk_tree_path_copy(path));
    }

    g_signal_connect(iRen, "activate",
                     G_CALLBACK(+[](GtkMenuItem *item, gpointer)
                                {
                                    auto *v = static_cast<FileManagerView *>(g_object_get_data(G_OBJECT(item), "view"));
                                    auto *p = static_cast<GtkTreePath *>(g_object_get_data(G_OBJECT(item), "path"));
                                    if (v->m_ctrl)
                                        v->m_ctrl->onRename(p);
                                    gtk_tree_path_free(p);
                                }),
                     nullptr);

    g_signal_connect(iCopy, "activate",
                     G_CALLBACK(+[](GtkMenuItem *item, gpointer)
                                {
                                    auto *v = static_cast<FileManagerView *>(g_object_get_data(G_OBJECT(item), "view"));
                                    auto *p = static_cast<GtkTreePath *>(g_object_get_data(G_OBJECT(item), "path"));
                                    if (v->m_ctrl)
                                        v->m_ctrl->onCopy(p);
                                    gtk_tree_path_free(p);
                                }),
                     nullptr);

    g_signal_connect(iCut, "activate",
                     G_CALLBACK(+[](GtkMenuItem *item, gpointer)
                                {
                                    auto *v = static_cast<FileManagerView *>(g_object_get_data(G_OBJECT(item), "view"));
                                    auto *p = static_cast<GtkTreePath *>(g_object_get_data(G_OBJECT(item), "path"));
                                    if (v->m_ctrl)
                                        v->m_ctrl->onCut(p);
                                    gtk_tree_path_free(p);
                                }),
                     nullptr);

    g_signal_connect(iDel, "activate",
                     G_CALLBACK(+[](GtkMenuItem *item, gpointer)
                                {
                                    auto *v = static_cast<FileManagerView *>(g_object_get_data(G_OBJECT(item), "view"));
                                    auto *p = static_cast<GtkTreePath *>(g_object_get_data(G_OBJECT(item), "path"));
                                    if (v->m_ctrl)
                                        v->m_ctrl->onDelete(p);
                                    gtk_tree_path_free(p);
                                }),
                     nullptr);

    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
    gtk_tree_path_free(path);
}
