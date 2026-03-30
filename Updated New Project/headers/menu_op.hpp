/**
 * @file menu_op.hpp
 * @brief SRP — Single Responsibility: context-menu operations only.
 *
 * In the original menu_op.c, rename, delete, copy, and cut were free functions
 * that mixed UI dialog code with file-system calls and direct struct mutation.
 *
 * MenuOperations groups only the two operations that are specific to the context
 * menu: rename and delete.  Copy and cut are delegated to ClipboardManager (ISP).
 *
 * SOLID principles demonstrated here:
 *   S (SRP) — Rename and delete only.  Clipboard is handled by IClipboard.
 *   D (DIP) — Depends on IProgressReporter and INavigable interfaces.
 */

#ifndef MENU_OPERATIONS_HPP
#define MENU_OPERATIONS_HPP

#include "interfaces.hpp"
#include "app_state.hpp"

/**
 * @brief Provides rename and delete actions for the right-click context menu.
 */
class MenuOperations
{
public:
    /**
     * @brief Constructs the menu operations handler.
     * @param state    Shared application state (parent window reference).
     * @param reporter Progress reporter for the delete operation.
     * @param nav      Navigation interface to refresh the directory after changes.
     */
    MenuOperations(AppState          *state,
                   IProgressReporter *reporter,
                   INavigable        *nav);

    /**
     * @brief Shows a rename dialog and renames the selected item.
     * @param treePath GTK tree path of the item to rename.
     */
    void rename(GtkTreePath *treePath);

    /**
     * @brief Confirms with the user and deletes the selected item.
     * @param treePath GTK tree path of the item to delete.
     */
    void deleteItem(GtkTreePath *treePath);

private:
    AppState          *m_state;
    IProgressReporter *m_reporter;
    INavigable        *m_nav;
};

#endif // MENU_OPERATIONS_HPP
