/**
 * @file menu_op.cpp
 * @brief SRP: rename and delete context-menu actions — nothing else.
 *        DIP: depends on IProgressReporter and INavigable interfaces.
 */

#include "menu.hpp"
#include "file_op.hpp"

#include <gtk/gtk.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

MenuOperations::MenuOperations(AppState          *state,
                               IProgressReporter *reporter,
                               INavigable        *nav)
    : m_state(state), m_reporter(reporter), m_nav(nav)
{}

// ── rename ────────────────────────────────────────────────────────────────────

void MenuOperations::rename(GtkTreePath *treePath)
{
    GtkTreeModel *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(m_state->treeview));

    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, treePath))
        return;

    gchar *oldName = nullptr;
    gtk_tree_model_get(model, &iter, 1, &oldName, -1);

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Rename", GTK_WINDOW(m_state->window), GTK_DIALOG_MODAL,
        "Ok",     GTK_RESPONSE_OK,
        "Cancel", GTK_RESPONSE_CANCEL,
        nullptr);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a new name");
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(area), entry);

    while (true)
    {
        gtk_widget_show_all(dialog);
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response != GTK_RESPONSE_OK)
            break;

        const gchar *newName = gtk_entry_get_text(GTK_ENTRY(entry));
        if (newName[0] == '\0')
            break;

        if (::rename(oldName, newName) != 0)
        {
            GtkWidget *alert = gtk_message_dialog_new(
                GTK_WINDOW(m_state->window), GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "%s", strerror(errno));
            gtk_dialog_run(GTK_DIALOG(alert));
            gtk_widget_destroy(alert);
            // Loop again so the user can try a different name.
        }
        else
        {
            m_nav->openDirectory();
            break;
        }
    }

    g_free(oldName);
    gtk_widget_destroy(dialog);
}

// ── deleteItem ────────────────────────────────────────────────────────────────

void MenuOperations::deleteItem(GtkTreePath *treePath)
{
    GtkTreeModel *model =
        gtk_tree_view_get_model(GTK_TREE_VIEW(m_state->treeview));

    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, treePath))
        return;

    gchar *fname = nullptr;
    gtk_tree_model_get(model, &iter, 1, &fname, -1);
    if (!fname) return;

    // Confirm deletion with the user.
    GtkWidget *confirm = gtk_message_dialog_new(
        GTK_WINDOW(m_state->window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Are you sure you want to delete '%s'?", fname);

    gint response = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);

    if (response != GTK_RESPONSE_YES)
    {
        g_free(fname);
        return;
    }

    // Build the full path from CWD + filename.
    char cwd[MAX_PATH];
    getcwd(cwd, sizeof(cwd));
    std::string fullPath = std::string(cwd) + "\\" + fname;

    // OCP + LSP: use DeleteOperation — no if/switch needed here.
    DeleteOperation op(fullPath);
    op.execute(m_reporter);

    m_nav->openDirectory();
    g_free(fname);
}
