/**
 * @file prog.cpp
 * @brief View layer — GTK progress window (IProgressReporter implementation).
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
    if (fraction > 1.0)
        fraction = 1.0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), fraction);

    gchar *text = g_strdup_printf("%.1f%%", fraction * 100.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_progressBar), text);
    g_free(text);

    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

void ProgressReporter::hide()
{
    if (m_window)
    {
        gtk_widget_destroy(m_window);
        m_window = nullptr;
        m_progressBar = nullptr;
    }
}
// void ProgressReporter::showCalculating()
// {
//     m_calcDialog = gtk_message_dialog_new(
//         m_parentWindow,                    // parent window (or nullptr if not set)
//         GTK_DIALOG_MODAL,
//         GTK_MESSAGE_INFO, GTK_BUTTONS_NONE,
//         "Calculating total size…\nPlease wait.");

//     gtk_window_set_title(GTK_WINDOW(m_calcDialog), "File Operation");
//     gtk_window_set_position(GTK_WINDOW(m_calcDialog), GTK_WIN_POS_CENTER_ON_PARENT);  // ← center on parent
//     gtk_dialog_set_response_sensitive(GTK_DIALOG(m_calcDialog), GTK_RESPONSE_OK, FALSE);
//     gtk_widget_show_all(m_calcDialog);

//     while (gtk_events_pending())
//         gtk_main_iteration_do(FALSE);
// }

void ProgressReporter::hideCalculating()
{
    if (m_calcDialog)
    {
        gtk_widget_destroy(m_calcDialog);
        m_calcDialog = nullptr;
    }
}
void ProgressReporter::set_parent_window(GtkWidget *parent)
{
    m_parentWindow = parent; // no cast needed
}

void ProgressReporter::showCalculating()
{
    m_calcDialog = gtk_message_dialog_new(
        GTK_WINDOW(m_parentWindow), // cast here when passing
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_NONE,
        "Calculating total size…\nPlease wait.");

    gtk_window_set_title(GTK_WINDOW(m_calcDialog), "File Operation");
    gtk_window_set_position(GTK_WINDOW(m_calcDialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(m_calcDialog), GTK_RESPONSE_OK, FALSE);
    gtk_widget_show_all(m_calcDialog);

    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}
