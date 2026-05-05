/**
 * @file controller.hpp
 * @brief MVC — Controller layer.
 *
 * The Controller is the mediator between user actions (View events) and
 * business logic (Model methods).  It:
 *
 *   1. Receives input events forwarded from the View's signal handlers.
 *   2. Translates them into Model method calls.
 *   3. Optionally queries the View for data needed by the Model call
 *      (e.g. selected filename, search query text).
 *
 * The Controller never manipulates GTK widgets directly — that is the View's
 * job.  It never contains file-system logic — that is the Model's job.
 *
 * SOLID principles:
 *   S (SRP) — The Controller handles user-event routing only.
 *   D (DIP) — Holds pointers to the abstract FileSystemModel and the
 *              abstract IModelObserver-conforming View; could be tested
 *              with mock implementations.
 */

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <gtk/gtk.h>
#include <string>

class FileSystemModel;
class FileManagerView;

/**
 * @brief Routes View events to Model method calls.
 */
class FileManagerController
{
public:
    /**
     * @param model The Model that owns all business logic and state.
     * @param view  The View that displays state and forwards input events.
     */
    FileManagerController(FileSystemModel *model, FileManagerView *view);

    // ── Input handlers (called by the View's signal trampolines) ──────────────

    /** User double-clicked (or pressed Enter) on a row. */
    void onItemActivated(GtkTreePath *path);

    /** User clicked the Back button. */
    void onBack();

    /** User clicked the Forward button. */
    void onForward();

    /** User clicked the New Folder button. */
    void onNewFolder();

    /** User clicked the Paste button. */
    void onPaste();

    /** User clicked the Search button. */
    void onSearch();

    /** User chose Rename from the context menu. */
    void onRename(GtkTreePath *path);

    /** User chose Copy from the context menu. */
    void onCopy(GtkTreePath *path);

    /** User chose Cut from the context menu. */
    void onCut(GtkTreePath *path);

    /** User chose Delete from the context menu. */
    void onDelete(GtkTreePath *path);

private:
    /**
     * @brief Shows a rename dialog and returns the new name, or "" if cancelled.
     */
    std::string promptRename(const std::string &oldName);

    /**
     * @brief Shows a new-folder name dialog and returns the chosen name,
     *        or "" if cancelled.
     */
    std::string promptFolderName();

    /**
     * @brief Shows a Yes/No confirmation dialog.
     * @return true if the user confirmed.
     */
    bool confirmDelete(const std::string &name);

    FileSystemModel *m_model;
    FileManagerView *m_view;
};

#endif // CONTROLLER_HPP
