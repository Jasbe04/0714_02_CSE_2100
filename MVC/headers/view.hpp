/**
 * @file view.hpp
 * @brief MVC — View layer.
 *
 * The View is responsible for ONE thing: displaying state that the Model
 * pushes to it via IModelObserver callbacks.  It holds all GTK widgets and
 * knows how to render FileEntry / ClipboardState data structures, but it
 * contains NO business logic and makes NO decisions about navigation.
 *
 * MVC contract:
 *   - The Model calls IModelObserver methods on the View.
 *   - The View never calls Model methods directly.
 *   - The Controller is the only mediator between View events and Model calls.
 *
 * SOLID principles:
 *   S (SRP) — The View builds the UI and renders model state only.
 *   D (DIP) — The View depends on the abstract IModelObserver interface;
 *              the Model never knows the concrete FileManagerView class.
 */

#ifndef VIEW_HPP
#define VIEW_HPP

#include "model.hpp"      // FileEntry, ClipboardState, IModelObserver
#include "interfaces.hpp" // IProgressReporter

#include <gtk/gtk.h>
#include <string>
#include <vector>

// Forward declaration — the Controller is wired in after construction.
class FileManagerController;

/**
 * @brief GTK View for the file manager.
 *
 * Implements IModelObserver so the Model can push new state without any
 * knowledge of GTK widgets.
 */
class FileManagerView : public IModelObserver
{
public:
    FileManagerView();
    ~FileManagerView();

    /**
     * @brief Wire the controller so signal handlers can forward events.
     *        Called once from main() after both View and Controller exist.
     */
    void setController(FileManagerController *ctrl);

    /**
     * @brief Build all GTK widgets (window, toolbar, tree-view, layout,
     *        signals).  Must be called before model.loadDrives() so that
     *        the observer callbacks have live widgets to render into.
     */
    void build();

    /**
     * @brief Show the window and enter the GTK event loop (blocking).
     *        Call this after model.loadDrives() has populated the drive list.
     */
    void run();

    // ── IModelObserver ────────────────────────────────────────────────────────

    void onDirectoryChanged(const std::vector<FileEntry> &entries,
                            const std::string            &currentPath) override;

    void onDrivesChanged(const std::vector<FileEntry> &drives) override;

    void onClipboardChanged(const ClipboardState &state) override;

    void onSearchResults(const std::vector<FileEntry> &results) override;

    // ── Accessors used by the Controller ─────────────────────────────────────

    GtkWidget    *window()       const { return m_window; }
    GtkWidget    *treeview()     const { return m_treeview; }
    GtkWidget    *searchEntry()  const { return m_searchEntry; }
    GdkPixbuf    *folderPixbuf() const { return m_iconPixbuf; }

    /**
     * @brief Returns the filename of the selected row, or "" if none.
     *        Column 1 holds the display name; column 2 holds the full path.
     */
    std::string selectedName()     const;
    std::string selectedFullPath() const;

    /** Returns the name at the given tree path (column 1). */
    std::string nameAtPath(GtkTreePath *path) const;

    /** Returns true when the drive-list is currently shown (depth == 0). */
    bool isDriveView() const { return m_isDriveView; }

private:
    // ── Widget builders ───────────────────────────────────────────────────────
    void loadPixbufs();
    void buildToolbar();
    void buildTreeView();
    void buildLayout();
    void connectSignals();

    // ── Rendering helpers ─────────────────────────────────────────────────────
    void populateStore(const std::vector<FileEntry> &entries);
    void setPathLabel(const std::string &text);
    void setForwardEnabled(bool enabled);
    void setBackEnabled(bool enabled);
    void setPasteEnabled(bool enabled);
    void setNewFolderEnabled(bool enabled);

    // ── Static GTK signal trampolines ─────────────────────────────────────────
    static void     onRowActivated    (GtkTreeView*, GtkTreePath*,
                                       GtkTreeViewColumn*, gpointer);
    static gboolean onButtonPress     (GtkTreeView*, GdkEventButton*, gpointer);
    static void     onBackClicked     (GtkWidget*, GdkEventButton*, gpointer);
    static void     onForwardClicked  (GtkWidget*, GdkEventButton*, gpointer);
    static void     onNewFolderClicked(GtkWidget*, GdkEventButton*, gpointer);
    static void     onPasteClicked    (GtkWidget*, GdkEventButton*, gpointer);
    static void     onSearchClicked   (GtkButton*, gpointer);

    // ── Right-click context menu ──────────────────────────────────────────────
    void showContextMenu(GtkTreeView *tv, GdkEventButton *event);

    // ── Widgets ───────────────────────────────────────────────────────────────
    GtkWidget    *m_window         = nullptr;
    GtkListStore *m_store          = nullptr;
    GtkWidget    *m_treeview       = nullptr;
    GtkWidget    *m_parentBox      = nullptr;
    GtkWidget    *m_fullBox        = nullptr;
    GtkWidget    *m_box            = nullptr;
    GtkWidget    *m_boxPath        = nullptr;
    GtkWidget    *m_boxSearch      = nullptr;
    GtkWidget    *m_labelPath      = nullptr;
    GtkWidget    *m_labelStatus    = nullptr;
    GtkWidget    *m_scrolledWindow = nullptr;
    GtkWidget    *m_searchEntry    = nullptr;
    GtkWidget    *m_searchButton   = nullptr;
    GtkWidget    *m_image1         = nullptr;   ///< Back icon.
    GtkWidget    *m_image2         = nullptr;   ///< Forward icon.
    GtkWidget    *m_event1         = nullptr;   ///< Back event box.
    GtkWidget    *m_event2         = nullptr;   ///< Forward event box.
    GtkWidget    *m_event3         = nullptr;   ///< New-folder event box.
    GtkWidget    *m_event4         = nullptr;   ///< Paste event box.

    // ── Pixbufs ───────────────────────────────────────────────────────────────
    GdkPixbuf *m_iconPixbuf      = nullptr;  ///< Folder icon.
    GdkPixbuf *m_scaledDriveIcon = nullptr;  ///< Drive icon (40×40).
    GdkPixbuf *m_pixbufBack      = nullptr;
    GdkPixbuf *m_pixbufNext      = nullptr;

    // ── Internal state ────────────────────────────────────────────────────────
    bool                   m_isDriveView = true;
    FileManagerController *m_ctrl        = nullptr;
};

#endif // VIEW_HPP
