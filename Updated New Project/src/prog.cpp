/**
 * @file prog.cpp
 * @brief Concrete GTK progress-window implementation of IProgressReporter.
 */

#include "prog.hpp"
#include <glib.h>

ProgressReporter::ProgressReporter() {}

ProgressReporter::~ProgressReporter()
{
    if (m_window)
        gtk_widget_destroy(m_window);
}

void ProgressReporter::show(const char *title)
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_window), title);
    gtk_window_set_default_size(GTK_WINDOW(m_window), 400, 60);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(m_window), 10);
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(m_window), GDK_WINDOW_TYPE_HINT_DIALOG);

    m_progressBar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(m_progressBar), TRUE);
    gtk_container_add(GTK_CONTAINER(m_window), m_progressBar);
    gtk_widget_show_all(m_window);
}

void ProgressReporter::update(double fraction)
{
    if (!m_progressBar)
        return;

    if (fraction > 1.0) fraction = 1.0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), fraction);

    gchar *text = g_strdup_printf("%.1f%%", fraction * 100.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_progressBar), text);
    g_free(text);

    // Pump the GTK event loop so the window stays responsive during file I/O.
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

void ProgressReporter::hide()
{
    if (m_window)
    {
        gtk_widget_destroy(m_window);
        m_window      = nullptr;
        m_progressBar = nullptr;
    }
}
