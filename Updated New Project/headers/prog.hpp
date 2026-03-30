/**
 * @file prog.hpp
 * @brief SRP — Single Responsibility: GTK progress-bar management only.
 *
 * In the original code, progress-bar updates were scattered across file_op.c,
 * menu_op.c, and prog.c.  ProgressReporter consolidates ALL progress-bar logic
 * in one place.
 *
 * SOLID principles demonstrated here:
 *   S (SRP)  — This class has one job: show/update/hide a progress window.
 *   D (DIP)  — FileCopier and FileDeleter depend on IProgressReporter, not on
 *               this concrete class, so they can be tested with a mock reporter.
 *   O (OCP)  — New reporter styles (e.g. a status-bar reporter) can be added
 *               by subclassing IProgressReporter without touching the callers.
 */

#ifndef PROGRESS_REPORTER_HPP
#define PROGRESS_REPORTER_HPP

#include "interfaces.hpp"
#include <gtk/gtk.h>

/**
 * @brief Concrete GTK implementation of IProgressReporter.
 *
 * Creates a small modal progress window and updates it during file operations.
 */
class ProgressReporter : public IProgressReporter
{
public:
    ProgressReporter();
    ~ProgressReporter() override;

    /**
     * @brief Creates and shows the progress window with the given title.
     * @param title Window title, e.g. "Copying..." or "Deleting...".
     */
    void show(const char *title) override;

    /**
     * @brief Updates the progress bar to @p fraction (0.0–1.0).
     *        Also pumps the GTK event loop to keep the UI responsive.
     */
    void update(double fraction) override;

    /** @brief Destroys the progress window. */
    void hide() override;

private:
    GtkWidget *m_window      = nullptr;
    GtkWidget *m_progressBar = nullptr;
};

#endif // PROGRESS_REPORTER_HPP
