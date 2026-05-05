/**
 * @file model.hpp
 * @brief MVC — Model layer.
 *
 * The Model owns ALL application data and business logic. It knows nothing
 * about GTK widgets or UI concerns. It exposes its state through plain data
 * types and notifies observers via IModelObserver callbacks.
 *
 * MVC responsibilities:
 *   Model   — filesystem state, navigation history, clipboard state,
 *              file operations (copy/move/delete/rename/search).
 *   View    — GTK widgets that DISPLAY model state (no logic).
 *   Controller — receives user input, calls Model methods, tells View to refresh.
 *
 * SOLID principles retained:
 *   S (SRP)  — Model holds data + business logic only; no UI code.
 *   O (OCP)  — FileOperation hierarchy still open for extension.
 *   D (DIP)  — Model depends on IProgressReporter, not concrete GTK reporter.
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include "interfaces.hpp"

#include <string>
#include <vector>
#include <stack>

// ── Data types exposed by the Model ──────────────────────────────────────────

/**
 * @brief Represents one visible entry in the current directory listing.
 */
struct FileEntry
{
    std::string name;       ///< Display name (filename).
    std::string fullPath;   ///< Absolute path (used to open/stat the file).
    bool        isDir;      ///< true if this is a directory.
};

/**
 * @brief Clipboard state exposed as a plain struct.
 */
struct ClipboardState
{
    std::string sourceDir;
    std::string sourceName;
    bool        isCopy = false;
    bool        isCut  = false;

    bool hasPending() const { return isCopy || isCut; }
};

// ── Observer interface (Model → View notification) ────────────────────────────

/**
 * @brief Interface the View implements so the Model can notify it of changes.
 *
 * The Model never holds a GTK widget; it calls these methods and lets the
 * View decide how to render the new state.
 */
class IModelObserver
{
public:
    /** Called when the directory listing has changed (navigate, refresh). */
    virtual void onDirectoryChanged(const std::vector<FileEntry> &entries,
                                    const std::string            &currentPath) = 0;

    /** Called when the drive list should be shown instead of a directory. */
    virtual void onDrivesChanged(const std::vector<FileEntry> &drives) = 0;

    /** Called when clipboard state changes (affects paste-button opacity). */
    virtual void onClipboardChanged(const ClipboardState &state) = 0;

    /** Called when a search completes with its results. */
    virtual void onSearchResults(const std::vector<FileEntry> &results) = 0;

    virtual ~IModelObserver() {}
};

// ── FileSystemModel ───────────────────────────────────────────────────────────

/**
 * @brief The Model: owns filesystem state and all business logic.
 *
 * The Controller calls methods on this class. After each state change the
 * Model notifies any registered IModelObserver (the View).
 */
class FileSystemModel
{
public:
    /**
     * @param reporter Used by file operations (copy/move/delete) for progress.
     */
    explicit FileSystemModel(IProgressReporter *reporter);

    // ── Observer registration ─────────────────────────────────────────────────
    void setObserver(IModelObserver *observer);

    // ── Navigation ────────────────────────────────────────────────────────────

    /** Load and broadcast the list of logical drives. */
    void loadDrives();

    /**
     * @brief Change into @p path and broadcast the new directory listing.
     * @param recordHistory  If true, push current dir onto the back stack.
     */
    void openPath(const std::string &path, bool recordHistory = true);

    /** Navigate to parent directory (or drive list if at root). */
    void goToParent();

    /** Navigate forward (re-enter the directory we just left). */
    void goForward();

    // ── File operations ───────────────────────────────────────────────────────

    /**
     * @brief Creates a new folder in the current directory.
     * @param name  Desired name; empty ⟹ auto-numbered "New Folder".
     * @return true on success, false if the name is already taken.
     */
    bool createFolder(const std::string &name);

    /** Mark @p name (in @p dir) for copying. */
    void markCopy(const std::string &dir, const std::string &name);

    /** Mark @p name (in @p dir) for moving (cut). */
    void markCut (const std::string &dir, const std::string &name);

    /** Execute the pending copy or move into the current directory. */
    void paste();

    /**
     * @brief Rename @p oldName to @p newName in the current directory.
     * @return true on success.
     */
    bool renameEntry(const std::string &oldName, const std::string &newName);

    /** Delete @p name from the current directory. */
    void deleteEntry(const std::string &name);

    // ── Search ────────────────────────────────────────────────────────────────

    /**
     * @brief Recursively search for @p query under the current directory.
     *        Results are broadcast via IModelObserver::onSearchResults().
     */
    void search(const std::string &query);

    // ── Accessors (View may read these for initial render) ────────────────────
    const std::string   &currentPath()    const { return m_currentPath; }
    const ClipboardState &clipboardState() const { return m_clipboard; }
    int                  depth()          const { return m_depth; }
    bool                 canGoForward()   const { return !m_forwardStack.empty(); }
    bool                 canGoBack()      const { return m_depth > 0; }

private:
    // ── Helpers ───────────────────────────────────────────────────────────────
    std::vector<FileEntry> listDirectory(const std::string &path) const;
    std::vector<FileEntry> listDrives()                           const;
    void searchRecursive(const std::string &dir,
                         const std::string &lowerQuery,
                         std::vector<FileEntry> &results)         const;

    void notifyDirectory();
    void notifyDrives();

    // ── State ─────────────────────────────────────────────────────────────────
    IProgressReporter      *m_reporter;
    IModelObserver         *m_observer    = nullptr;
    std::string             m_currentPath;
    int                     m_depth       = 0;

    ClipboardState          m_clipboard;

    std::stack<std::string> m_backStack;
    std::stack<std::string> m_forwardStack;
};

#endif // MODEL_HPP
