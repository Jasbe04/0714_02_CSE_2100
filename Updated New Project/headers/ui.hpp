/**
 * @file ui.hpp
 * @brief DIP — Dependency Inversion Principle.
 *        SRP — Single Responsibility: build and wire up the UI only.
 *
 * In the original ui.c, the set_ui() function:
 *   - Built all widgets
 *   - Knew about file_op.c functions directly (create_new_folder, copy_function)
 *   - Knew about nvign.c functions directly (show_drives, go_to_parent_folder)
 *
 * UIController fixes this by depending ONLY on abstractions (interfaces):
 *   - INavigable  for navigation
 *   - IClipboard  for copy/cut/paste
 *   - ISearchable for search
 *
 * This means swapping the real NavigationManager for a mock (in tests) requires
 * zero changes to UIController.
 *
 * SOLID principles demonstrated here:
 *   D (DIP) — UIController depends on INavigable, IClipboard, ISearchable.
 *              It does NOT depend on NavigationManager, ClipboardManager,
 *              SearchService directly.
 *   S (SRP) — UIController builds and wires the UI.  It does NOT implement
 *              navigation, file ops, or search.
 */

#ifndef UI_CONTROLLER_HPP
#define UI_CONTROLLER_HPP

#include "interfaces.hpp"
#include "app_state.hpp"
#include "menu.hpp"
#include "folder_creator.hpp"

/**
 * @brief Builds the GTK file-manager window and wires all signal handlers.
 *
 * Depends only on the four interfaces — never on concrete service classes.
 */
class UIController
{
public:
    /**
     * @brief Constructs the controller with all required dependencies.
     *
     * @param state     Shared application state (populated by buildUI).
     * @param nav       Navigation service (INavigable).
     * @param clipboard Clipboard service (IClipboard).
     * @param search    Search service (ISearchable).
     * @param menu      Context-menu operations.
     * @param folder    Folder creation service.
     */
    UIController(AppState       *state,
                 INavigable     *nav,
                 IClipboard     *clipboard,
                 ISearchable    *search,
                 MenuOperations *menu,
                 FolderCreator  *folder);

    /**
     * @brief Builds all GTK widgets, connects signals, and starts the event loop.
     *        Equivalent to the original set_ui().
     */
    void buildAndRun();

private:
    // ── Signal-handler trampolines ────────────────────────────────────────────
    // GTK requires plain C functions for g_signal_connect, so we use static
    // methods as trampolines that cast user-data back to UIController* and
    // forward to member functions.

    static void         onRowActivated   (GtkTreeView*, GtkTreePath*,
                                          GtkTreeViewColumn*, gpointer);
    static gboolean     onButtonPress    (GtkTreeView*, GdkEventButton*, gpointer);
    static void         onBackClicked    (GtkWidget*, GdkEventButton*, gpointer);
    static void         onForwardClicked (GtkWidget*, GdkEventButton*, gpointer);
    static void         onNewFolderClicked(GtkWidget*, GdkEventButton*, gpointer);
    static void         onPasteClicked   (GtkWidget*, GdkEventButton*, gpointer);
    static void         onSearchClicked  (GtkButton*, gpointer);

    // ── Member handlers ───────────────────────────────────────────────────────
    void handleRowActivated(GtkTreePath *path);
    void handleRightClick  (GtkTreeView *tv, GdkEventButton *event);

    // ── Private helpers ───────────────────────────────────────────────────────
    void loadPixbufs();
    void buildToolbar();
    void buildTreeView();
    void connectSignals();

    // ── Dependencies (all interfaces) ────────────────────────────────────────
    AppState       *m_state;
    INavigable     *m_nav;
    IClipboard     *m_clipboard;
    ISearchable    *m_search;
    MenuOperations *m_menu;
    FolderCreator  *m_folder;
};

#endif // UI_CONTROLLER_HPP
