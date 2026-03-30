/**
 * @file main.cpp
 * @brief Application entry point — Dependency Injection root.
 *
 * This is the "composition root": the only place in the application where
 * concrete classes are instantiated and wired together.  Every other module
 * depends on interfaces, not on the concrete types created here.
 *
 * SOLID principles demonstrated here:
 *   D (DIP) — All dependencies are injected through constructors.
 *              UIController never calls `new NavigationManager` itself.
 *   S (SRP) — main() only creates objects and starts the app.
 *
 * Dependency graph (arrows = "depends on interface of"):
 *
 *   main()
 *     ├── AppState           (plain data, no logic)
 *     ├── ProgressReporter   → IProgressReporter
 *     ├── IconLoader         (no interface needed; pure utility)
 *     ├── NavigationManager  → INavigable
 *     ├── ClipboardManager   → IClipboard  (uses INavigable, IProgressReporter)
 *     ├── SearchService      → ISearchable
 *     ├── MenuOperations     (uses IProgressReporter, INavigable)
 *     ├── FolderCreator      (no interface needed; simple dialog)
 *     └── UIController       (depends ONLY on interfaces above)
 */

#include <gtk/gtk.h>

#include "app_state.hpp"
#include "interfaces.hpp"
#include "prog.hpp"
#include "icon.hpp"
#include "nvign.hpp"
#include "menu.hpp"
#include "search.hpp"
#include "menu_op.hpp"
#include "folder_creator.hpp"
#include "ui.hpp"

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // ── 1. Shared state (SRP: only holds data) ────────────────────────────────
    AppState state;

    // ── 2. Leaf services (no dependencies on other services) ─────────────────
    ProgressReporter reporter;   // IProgressReporter implementation
    IconLoader       icons;      // Pure utility — no interface needed

    // ── 3. Navigation (depends on AppState + IconLoader) ─────────────────────
    //    UIController will receive this as INavigable* — DIP satisfied.
    NavigationManager nav(&state, &icons);

    // ── 4. Clipboard (depends on IProgressReporter + INavigable) ─────────────
    //    Injecting the interface pointers, not the concrete types.
    ClipboardManager clipboard(&state, &reporter, &nav);

    // ── 5. Search (depends on AppState + IconLoader) ──────────────────────────
    SearchService search(&state, &icons);

    // ── 6. Context-menu operations (depends on IProgressReporter + INavigable)
    MenuOperations menuOps(&state, &reporter, &nav);

    // ── 7. Folder creation (simple dialog helper) ────────────────────────────
    FolderCreator folderCreator;

    // ── 8. UI Controller — depends ONLY on interfaces (DIP) ──────────────────
    UIController ui(
        &state,
        &nav,           // INavigable*
        &clipboard,     // IClipboard*
        &search,        // ISearchable*
        &menuOps,
        &folderCreator
    );

    // ── 9. Build the window and start the GTK event loop ─────────────────────
    ui.buildAndRun();

    return 0;
}
