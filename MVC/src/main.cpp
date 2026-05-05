/**
 * @file main.cpp
 * @brief MVC Composition Root — the only place concrete classes are created.
 *
 * Construction order:
 *   1. ProgressReporter  — View-layer service for Model's file operations.
 *   2. FileSystemModel   — The Model (owns all state + business logic).
 *   3. FileManagerView   — The View (owns all GTK widgets).
 *   4. FileManagerController — The Controller (routes events Model ↔ View).
 *   5. Wire them together (setObserver, setController).
 *   6. view.build()      — Create all GTK widgets (no event loop yet).
 *   7. model.loadDrives()— Populate drive list; View widgets now exist to
 *                          receive the onDrivesChanged() observer callback.
 *   8. view.run()        — Show window and start the GTK event loop.
 *
 * MVC data flow:
 *
 *   User input
 *       │
 *       ▼
 *   FileManagerView (GTK signal)
 *       │  forwards event
 *       ▼
 *   FileManagerController
 *       │  calls business logic
 *       ▼
 *   FileSystemModel
 *       │  notifies via IModelObserver
 *       ▼
 *   FileManagerView (renders new state)
 *
 * SOLID:
 *   D (DIP) — All dependencies are injected; nothing instantiates its own deps.
 *   S (SRP) — main() only creates objects and starts the app.
 */

#include <gtk/gtk.h>

#include "interfaces.hpp" // IProgressReporter
#include "prog.hpp"       // ProgressReporter  (View layer)
#include "model.hpp"      // FileSystemModel   (Model layer)
#include "view.hpp"       // FileManagerView   (View layer)
#include "controller.hpp" // FileManagerController (Controller layer)

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    ProgressReporter reporter;

    FileSystemModel model(&reporter);
    FileManagerView view;
    FileManagerController controller(&model, &view);

    model.setObserver(&view);
    view.setController(&controller);

    view.build(); // <-- creates m_window inside the view

    // --- NEW: set reporter's parent window to the view's main window ---
    reporter.set_parent_window(view.window()); // <-- add this line

    model.loadDrives();
    view.run();

    return 0;
}
