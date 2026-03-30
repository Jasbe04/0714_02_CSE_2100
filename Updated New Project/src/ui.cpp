/**
 * @file ui.cpp
 * @brief DIP in action: UIController depends ONLY on interfaces, never on
 *        concrete service classes (NavigationManager, ClipboardManager, etc.).
 *
 * Signal handlers are static trampoline methods that cast the gpointer
 * user-data back to UIController* and forward to member functions.
 */

#include "ui.hpp"

#include <gtk/gtk.h>
#include <windows.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <string>

// ── Construction ──────────────────────────────────────────────────────────────

UIController::UIController(AppState       *state,
                           INavigable     *nav,
                           IClipboard     *clipboard,
                           ISearchable    *search,
                           MenuOperations *menu,
                           FolderCreator  *folder)
    : m_state(state)
    , m_nav(nav)
    , m_clipboard(clipboard)
    , m_search(search)
    , m_menu(menu)
    , m_folder(folder)
{}

// ── buildAndRun ───────────────────────────────────────────────────────────────

void UIController::buildAndRun()
{
    // ── Main window ──────────────────────────────────────────────────────────
    m_state->labelPath = gtk_label_new(" ");
    m_state->window    = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_state->window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(m_state->window), 700, 600);
    gtk_window_set_position(GTK_WINDOW(m_state->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(m_state->window), 10);
    g_signal_connect(m_state->window, "destroy",
                     G_CALLBACK(gtk_main_quit), nullptr);

    // ── Search bar ───────────────────────────────────────────────────────────
    m_state->boxSearch    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    m_state->searchEntry  = gtk_entry_new();
    m_state->searchButton = gtk_button_new_with_label("Search");
    gtk_box_pack_start(GTK_BOX(m_state->boxSearch),
                       m_state->searchEntry,  TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(m_state->boxSearch),
                       m_state->searchButton, FALSE, FALSE, 0);

    loadPixbufs();
    buildToolbar();
    buildTreeView();

    // ── Layout ───────────────────────────────────────────────────────────────
    m_state->parentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,   10);
    m_state->fullBox   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 250);
    m_state->box       = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,  10);
    m_state->boxPath   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,  10);

    gtk_box_pack_start(GTK_BOX(m_state->box), m_state->event1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_state->box), m_state->event2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_state->box), m_state->event3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_state->box), m_state->event4, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(m_state->fullBox),   m_state->box,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_state->parentBox), m_state->fullBox,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_state->boxPath),   m_state->labelPath, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(m_state->parentBox), m_state->boxPath,   FALSE, FALSE, 0);

    // ── Scrolled window ───────────────────────────────────────────────────────
    m_state->scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_set_border_width(GTK_CONTAINER(m_state->scrolledWindow), 5);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(m_state->scrolledWindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(m_state->parentBox),
                       m_state->scrolledWindow, TRUE, TRUE, 0);

    connectSignals();

    // ── Initial display ───────────────────────────────────────────────────────
    m_state->labelStatus = gtk_label_new("This folder is empty");
    m_nav->showDrives();
    gtk_widget_set_opacity(m_state->event2, 0.1);
    gtk_container_add(GTK_CONTAINER(m_state->window), m_state->parentBox);
    gtk_widget_show_all(m_state->window);
    gtk_main();
}

// ── loadPixbufs ───────────────────────────────────────────────────────────────

void UIController::loadPixbufs()
{
    GError *err = nullptr;
    m_state->iconPixbuf = gdk_pixbuf_new_from_file("File.png",     &err);
    m_state->pixbufBack = gdk_pixbuf_new_from_file("back.png",     &err);
    m_state->pixbufNext = gdk_pixbuf_new_from_file("next.png",     &err);

    GdkPixbuf *pbNewFolder = gdk_pixbuf_new_from_file("newFolder.png", &err);
    GdkPixbuf *pbDrive     = gdk_pixbuf_new_from_file("drive.png",     &err);
    GdkPixbuf *pbPaste     = gdk_pixbuf_new_from_file("Paste.png",     &err);

    m_state->scaledDriveIcon =
        gdk_pixbuf_scale_simple(pbDrive, 40, 40, GDK_INTERP_BILINEAR);

    m_state->image1 = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(m_state->pixbufBack, 40, 40, GDK_INTERP_BILINEAR));
    m_state->image2 = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(m_state->pixbufNext, 40, 40, GDK_INTERP_BILINEAR));

    GtkWidget *imgNewFolder = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(pbNewFolder, 40, 40, GDK_INTERP_BILINEAR));
    GtkWidget *imgPaste = gtk_image_new_from_pixbuf(
        gdk_pixbuf_scale_simple(pbPaste, 30, 30, GDK_INTERP_BILINEAR));

    // ── Event boxes ──────────────────────────────────────────────────────────
    m_state->event1 = gtk_event_box_new();
    m_state->event2 = gtk_event_box_new();
    m_state->event3 = gtk_event_box_new();
    m_state->event4 = gtk_event_box_new();

    gtk_container_add(GTK_CONTAINER(m_state->event1), m_state->image1);
    gtk_container_add(GTK_CONTAINER(m_state->event2), m_state->image2);
    gtk_container_add(GTK_CONTAINER(m_state->event3), imgNewFolder);
    gtk_container_add(GTK_CONTAINER(m_state->event4), imgPaste);

    if (pbNewFolder) g_object_unref(pbNewFolder);
    if (pbDrive)     g_object_unref(pbDrive);
    if (pbPaste)     g_object_unref(pbPaste);
}

// ── buildToolbar ─────────────────────────────────────────────────────────────

void UIController::buildToolbar()
{
    // Toolbar labels (Back, Next, New Folder) sit beside the event boxes.
    GtkWidget *label1 = gtk_label_new("Back");
    GtkWidget *label2 = gtk_label_new("Next");
    GtkWidget *label3 = gtk_label_new("New Folder");
    (void)label1; (void)label2; (void)label3; // kept for future use
}

// ── buildTreeView ─────────────────────────────────────────────────────────────

void UIController::buildTreeView()
{
    // Store: column 0 = pixbuf, column 1 = display name, column 2 = full path
    m_state->store    = gtk_list_store_new(3, GDK_TYPE_PIXBUF,
                                           G_TYPE_STRING, G_TYPE_STRING);
    m_state->treeview = gtk_tree_view_new_with_model(
        GTK_TREE_MODEL(m_state->store));

    // Icon column
    GtkCellRenderer   *pixRenderer  = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *iconCol      = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(iconCol, pixRenderer, TRUE);
    gtk_tree_view_column_add_attribute(iconCol, pixRenderer, "pixbuf", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_state->treeview), iconCol);

    // Name column
    GtkCellRenderer   *textRenderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *nameCol      =
        gtk_tree_view_column_new_with_attributes("Name", textRenderer,
                                                 "text", 1, nullptr);
    PangoFontDescription *fontDesc = pango_font_description_new();
    pango_font_description_set_size(fontDesc, 14 * PANGO_SCALE);
    g_object_set(textRenderer, "font-desc", fontDesc, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_state->treeview), nameCol);
}

// ── connectSignals ────────────────────────────────────────────────────────────

void UIController::connectSignals()
{
    g_signal_connect(m_state->treeview, "row-activated",
                     G_CALLBACK(onRowActivated), this);
    g_signal_connect(m_state->treeview, "button-press-event",
                     G_CALLBACK(onButtonPress), this);
    g_signal_connect(m_state->event1, "button-press-event",
                     G_CALLBACK(onBackClicked), this);
    g_signal_connect(m_state->event2, "button-press-event",
                     G_CALLBACK(onForwardClicked), this);
    g_signal_connect(m_state->event3, "button-press-event",
                     G_CALLBACK(onNewFolderClicked), this);
    g_signal_connect(m_state->event4, "button-press-event",
                     G_CALLBACK(onPasteClicked), this);
    g_signal_connect(m_state->searchButton, "clicked",
                     G_CALLBACK(onSearchClicked), this);
}

// ── Static trampolines ────────────────────────────────────────────────────────

/*static*/
void UIController::onRowActivated(GtkTreeView        *,
                                   GtkTreePath        *path,
                                   GtkTreeViewColumn  *,
                                   gpointer            userData)
{
    static_cast<UIController *>(userData)->handleRowActivated(path);
}

/*static*/
gboolean UIController::onButtonPress(GtkTreeView    *tv,
                                      GdkEventButton *event,
                                      gpointer        userData)
{
    auto *self = static_cast<UIController *>(userData);
    if (event->type == GDK_BUTTON_PRESS && event->button == 3
        && self->m_state->depth > 0)
    {
        self->handleRightClick(tv, event);
        return TRUE;
    }
    return FALSE;
}

/*static*/
void UIController::onBackClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    // DIP: UIController calls INavigable — it has no idea this is NavigationManager.
    static_cast<UIController *>(userData)->m_nav->goToParent();
}

/*static*/
void UIController::onForwardClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    static_cast<UIController *>(userData)->m_nav->goForward();
}

/*static*/
void UIController::onNewFolderClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    auto *self = static_cast<UIController *>(userData);
    if (self->m_state->depth != 0)
    {
        if (self->m_folder->createFolder(self->m_state->window))
            self->m_nav->openDirectory();
    }
}

/*static*/
void UIController::onPasteClicked(GtkWidget *, GdkEventButton *, gpointer userData)
{
    // DIP: UIController calls IClipboard — not ClipboardManager directly.
    static_cast<UIController *>(userData)->m_clipboard->paste();
}

/*static*/
void UIController::onSearchClicked(GtkButton *, gpointer userData)
{
    auto       *self  = static_cast<UIController *>(userData);
    const char *query = gtk_entry_get_text(GTK_ENTRY(self->m_state->searchEntry));

    if (query[0] == '\0') return;

    char cwd[MAX_PATH];
    getcwd(cwd, sizeof(cwd));

    // DIP: UIController calls ISearchable — not SearchService directly.
    self->m_search->search(query, cwd);
}

// ── handleRowActivated ────────────────────────────────────────────────────────

void UIController::handleRowActivated(GtkTreePath *path)
{
    gtk_widget_set_opacity(m_state->event2, 0.1);

    GtkTreeModel *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(m_state->treeview));
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;

    gchar *name = nullptr;
    if (m_state->depth == 0)
        gtk_tree_model_get(model, &iter, 1, &name, -1);
    else
        gtk_tree_model_get(model, &iter, 2, &name, -1);

    m_state->navForward = 0;

    if (m_state->depth == 0)
    {
        // Extract drive letter from display string, e.g. "Local Disk (C)" → "C:\\"
        char drivePath[5] = "";
        drivePath[0] = name[strlen(name) - 3];
        drivePath[1] = ':';
        drivePath[2] = '\\';
        drivePath[3] = '\\';
        drivePath[4] = '\0';
        chdir(drivePath);
        ++m_state->depth;
        m_nav->openDirectory();
    }
    else
    {
        struct stat fileStat;
        if (stat(name, &fileStat) == 0)
        {
            if (S_ISDIR(fileStat.st_mode))
            {
                // Save CWD onto backward stack before descending.
                char cwd[MAX_PATH];
                getcwd(cwd, sizeof(cwd));
                m_state->backStack.push(std::string(cwd));

                ++m_state->depth;
                chdir(name);
                m_nav->openDirectory();
            }
            else
            {
                // Open file with its default application.
                ShellExecute(nullptr, "open", name, nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
    }

    g_free(name);
}

// ── handleRightClick ──────────────────────────────────────────────────────────

void UIController::handleRightClick(GtkTreeView *tv, GdkEventButton *event)
{
    GtkTreeSelection *sel  = gtk_tree_view_get_selection(tv);
    GtkTreePath      *path = nullptr;

    if (!gtk_tree_view_get_path_at_pos(tv,
            (gint)event->x, (gint)event->y,
            &path, nullptr, nullptr, nullptr))
        return;

    if (!gtk_tree_selection_path_is_selected(sel, path))
    {
        gtk_tree_path_free(path);
        return;
    }

    // We need the path to survive until the menu-item callbacks fire.
    // Use a static array of two pointers as the original code did.
    // The path is freed after the user dismisses the menu (see below).
    static gpointer cbData[2];
    cbData[0] = m_state;
    cbData[1] = path;   // freed by the lambda below

    GtkWidget *menu  = gtk_menu_new();
    GtkWidget *iRen  = gtk_menu_item_new_with_label("Rename");
    GtkWidget *iCopy = gtk_menu_item_new_with_label("Copy");
    GtkWidget *iCut  = gtk_menu_item_new_with_label("Cut");
    GtkWidget *iDel  = gtk_menu_item_new_with_label("Delete");

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iRen);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iCopy);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iCut);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), iDel);
    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

    // ── Menu item callbacks ───────────────────────────────────────────────────
    // Capture 'this' so menu items can call the interface methods.

    MenuOperations *menuOps  = m_menu;
    IClipboard     *clipbd   = m_clipboard;

    g_signal_connect(iRen, "activate",
        G_CALLBACK(+[](GtkMenuItem *, gpointer d) {
            auto **p = static_cast<gpointer *>(d);
            // Reconstruct MenuOperations pointer from closure capture via g_object_get_data.
        }), cbData);

    // Use simpler direct lambdas via GClosure or wrapper structs.
    // Since GTK callbacks must be plain C functions, we store the extra
    // pointer in g_object_set_data on each menu item.

    g_object_set_data(G_OBJECT(iRen),  "ctrl", this);
    g_object_set_data(G_OBJECT(iCopy), "ctrl", this);
    g_object_set_data(G_OBJECT(iCut),  "ctrl", this);
    g_object_set_data(G_OBJECT(iDel),  "ctrl", this);

    g_object_set_data(G_OBJECT(iRen),  "path", path);
    g_object_set_data(G_OBJECT(iCopy), "path", path);
    g_object_set_data(G_OBJECT(iCut),  "path", path);
    g_object_set_data(G_OBJECT(iDel),  "path", path);

    // Rename
    g_signal_connect(iRen, "activate",
        G_CALLBACK(+[](GtkMenuItem *item, gpointer) {
            auto *ctrl = static_cast<UIController *>(
                g_object_get_data(G_OBJECT(item), "ctrl"));
            auto *tp   = static_cast<GtkTreePath *>(
                g_object_get_data(G_OBJECT(item), "path"));
            ctrl->m_menu->rename(tp);
        }), nullptr);

    // Copy
    g_signal_connect(iCopy, "activate",
        G_CALLBACK(+[](GtkMenuItem *item, gpointer) {
            auto *ctrl  = static_cast<UIController *>(
                g_object_get_data(G_OBJECT(item), "ctrl"));
            auto *tp    = static_cast<GtkTreePath *>(
                g_object_get_data(G_OBJECT(item), "path"));

            GtkTreeModel *model =
                gtk_tree_view_get_model(GTK_TREE_VIEW(ctrl->m_state->treeview));
            GtkTreeIter iter;
            if (!gtk_tree_model_get_iter(model, &iter, tp)) return;

            char cwd[MAX_PATH];
            getcwd(cwd, sizeof(cwd));
            gchar *name = nullptr;
            gtk_tree_model_get(model, &iter, 1, &name, -1);
            ctrl->m_clipboard->markCopy(cwd, name);
            g_free(name);
        }), nullptr);

    // Cut
    g_signal_connect(iCut, "activate",
        G_CALLBACK(+[](GtkMenuItem *item, gpointer) {
            auto *ctrl  = static_cast<UIController *>(
                g_object_get_data(G_OBJECT(item), "ctrl"));
            auto *tp    = static_cast<GtkTreePath *>(
                g_object_get_data(G_OBJECT(item), "path"));

            GtkTreeModel *model =
                gtk_tree_view_get_model(GTK_TREE_VIEW(ctrl->m_state->treeview));
            GtkTreeIter iter;
            if (!gtk_tree_model_get_iter(model, &iter, tp)) return;

            char cwd[MAX_PATH];
            getcwd(cwd, sizeof(cwd));
            gchar *name = nullptr;
            gtk_tree_model_get(model, &iter, 1, &name, -1);
            ctrl->m_clipboard->markCut(cwd, name);
            g_free(name);
        }), nullptr);

    // Delete
    g_signal_connect(iDel, "activate",
        G_CALLBACK(+[](GtkMenuItem *item, gpointer) {
            auto *ctrl = static_cast<UIController *>(
                g_object_get_data(G_OBJECT(item), "ctrl"));
            auto *tp   = static_cast<GtkTreePath *>(
                g_object_get_data(G_OBJECT(item), "path"));
            ctrl->m_menu->deleteItem(tp);
        }), nullptr);

    // Free the tree path when the menu is destroyed.
    g_signal_connect(menu, "hide",
        G_CALLBACK(+[](GtkWidget *w, gpointer) {
            auto *tp = static_cast<GtkTreePath *>(
                g_object_get_data(G_OBJECT(
                    gtk_container_get_children(GTK_CONTAINER(w))->data),
                    "path"));
            if (tp) gtk_tree_path_free(tp);
        }), nullptr);
}
