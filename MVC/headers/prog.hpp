/**
 * @file prog.hpp
 * @brief View layer — GTK concrete implementation of IProgressReporter.
 *
 * In MVC terms this is part of the View: it creates a GTK window to show
 * progress. The Model never sees this class — it only calls IProgressReporter*.
 *
 * SOLID:
 *   S (SRP) — One job: show/update/hide a progress window.
 *   D (DIP) — Model depends on IProgressReporter, not this concrete class.
 *   O (OCP) — A status-bar reporter can be added without touching the Model.
 */

#ifndef PROGRESS_REPORTER_HPP
#define PROGRESS_REPORTER_HPP

#include "interfaces.hpp"
#include <gtk/gtk.h>

class ProgressReporter : public IProgressReporter
{
public:
    ProgressReporter();
    ~ProgressReporter() override;
    void showCalculating() override;
    void hideCalculating() override;
    void set_parent_window(GtkWidget *parent);

    void show  (const char *title)  override;
    void update(double      fraction) override;
    void hide  ()                   override;

private:
    GtkWidget *m_window      = nullptr;
    GtkWidget *m_progressBar = nullptr;
    GtkWidget *m_calcDialog  = nullptr;
    GtkWidget *m_calcLabel   = nullptr;
     GtkWidget *m_parentWindow = nullptr;
};

#endif // PROGRESS_REPORTER_HPP
