/**
 * @file app_state.hpp
 * @brief SRP — Single Responsibility: central application state only.
 *
 * In the original C code, the Data struct held everything: UI widgets, pixbufs,
 * navigation stacks, and clipboard state all mixed together.
 *
 * Here AppState is responsible for ONE thing: storing application-wide state.
 * It is NOT responsible for navigation logic, file operations, or UI building.
 * Those concerns live in their own classes.
 *
 * SOLID principles demonstrated here:
 *   S (SRP) — AppState only holds state; no logic.
 *   I (ISP) — Modules receive only AppState fields they need via the interfaces.
 */

#ifndef APP_STATE_HPP
#define APP_STATE_HPP

#include <gtk/gtk.h>
#include <windows.h>
#include <stack>
#include <string>

/**
 * @brief Holds all UI widgets, pixbufs, and navigation state for the
 *        file manager.  A single heap-allocated instance is passed by pointer
 *        to every callback.
 *
 *  Unlike the original C struct, the navigation stacks are now proper
 *  std::stack<std::string> objects, removing all manual linked-list management.
 */
struct AppState
{
    // ── Pixbufs ──────────────────────────────────────────────────────────────
    GdkPixbuf *iconPixbuf      = nullptr;  ///< Default folder icon.
    GdkPixbuf *scaledDriveIcon = nullptr;  ///< Scaled drive icon (40×40).
    GdkPixbuf *pixbufBack      = nullptr;  ///< Raw back-button pixbuf.
    GdkPixbuf *pixbufNext      = nullptr;  ///< Raw next-button pixbuf.

    // ── Top-level window and core containers ─────────────────────────────────
    GtkWidget    *window         = nullptr;
    GtkListStore *store          = nullptr;
    GtkWidget    *treeview       = nullptr;
    GtkWidget    *parentBox      = nullptr;
    GtkWidget    *labelStatus    = nullptr;
    GtkWidget    *labelPath      = nullptr;
    GtkWidget    *boxSearch      = nullptr;
    GtkWidget    *boxPath        = nullptr;
    GtkWidget    *box            = nullptr;
    GtkWidget    *fullBox        = nullptr;
    GtkWidget    *image1         = nullptr;
    GtkWidget    *image2         = nullptr;
    GtkWidget    *scrolledWindow = nullptr;
    GtkWidget    *searchEntry    = nullptr;
    GtkWidget    *searchButton   = nullptr;

    // ── Toolbar event boxes ───────────────────────────────────────────────────
    GtkWidget *event1 = nullptr;  ///< Back button.
    GtkWidget *event2 = nullptr;  ///< Next button.
    GtkWidget *event3 = nullptr;  ///< New-Folder button.
    GtkWidget *event4 = nullptr;  ///< Paste button.

    // ── Clipboard state ───────────────────────────────────────────────────────
    std::string clipboardDir;   ///< Directory of the copied/cut item.
    std::string clipboardName;  ///< Name of the copied/cut item.
    bool        isCopy = false;
    bool        isCut  = false;

    // ── Navigation state ──────────────────────────────────────────────────────
    int depth      = 0;  ///< 0 = drive list view.
    int navForward = 0;  ///< How many directories can be revisited forward.

    // ── Navigation stacks (replaces manual linked lists) ─────────────────────
    std::stack<std::string> backStack;     ///< Backward-navigation history.
    std::stack<std::string> forwardStack;  ///< Forward-navigation history.
};

#endif // APP_STATE_HPP
