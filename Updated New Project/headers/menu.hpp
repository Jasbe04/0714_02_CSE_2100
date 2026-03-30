/**
 * @file menu.hpp
 * @brief SRP — Single Responsibility: clipboard (copy/cut/paste) only.
 *        ISP — Implements IClipboard; navigation and search never see this.
 *
 * In the original code, copy() and cut() in menu_op.c wrote directly into the
 * Data struct, while copy_function() in file_op.c read from it.  The clipboard
 * "protocol" was implicit and scattered across files.
 *
 * ClipboardManager owns the clipboard state and orchestrates paste using the
 * FileOperation hierarchy (OCP + LSP).
 *
 * SOLID principles demonstrated here:
 *   S (SRP)  — Manages clipboard state and paste execution only.
 *   I (ISP)  — Implements IClipboard; UIController depends on this interface.
 *   O (OCP)  — Uses FileOperation polymorphism so new paste strategies need
 *               only a new FileOperation subclass.
 *   D (DIP)  — Depends on IProgressReporter, not the concrete GTK reporter.
 */

#ifndef CLIPBOARD_MANAGER_HPP
#define CLIPBOARD_MANAGER_HPP

#include "interfaces.hpp"
#include "app_state.hpp"
#include <string>

/**
 * @brief Manages copy/cut/paste clipboard operations.
 */
class ClipboardManager : public IClipboard
{
public:
    /**
     * @brief Constructs the manager.
     * @param state    Shared application state (for paste-button opacity).
     * @param reporter Progress reporter shown during paste.
     * @param nav      Navigation interface used to refresh the directory after paste.
     */
    ClipboardManager(AppState          *state,
                     IProgressReporter *reporter,
                     INavigable        *nav);

    // ── IClipboard ────────────────────────────────────────────────────────────

    /**
     * @brief Records @p name in @p dir as the item to be copied.
     */
    void markCopy(const char *dir, const char *name) override;

    /**
     * @brief Records @p name in @p dir as the item to be moved (cut).
     */
    void markCut(const char *dir, const char *name) override;

    /**
     * @brief Executes the pending copy or move into the current working directory.
     *        Uses CopyOperation or MoveOperation from the FileOperation hierarchy.
     */
    void paste() override;

    /** @brief Returns true if a copy or cut is pending. */
    bool hasPending() const override;

private:
    AppState          *m_state;
    IProgressReporter *m_reporter;
    INavigable        *m_nav;
};

#endif // CLIPBOARD_MANAGER_HPP
