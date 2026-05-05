/**
 * @file controller.cpp
 * @brief MVC Controller implementation.
 *
 * The Controller is a thin routing layer.  For each user-input event it:
 *   1. Reads the minimal information it needs from the View (selected item, text).
 *   2. Shows any necessary dialog (rename/confirm prompts — these are UI-only
 *      decision points that belong here, not in the Model).
 *   3. Calls the corresponding Model method.
 *
 * The Controller does NOT:
 *   - Manipulate GTK widgets directly (that is the View's job).
 *   - Contain filesystem logic (that is the Model's job).
 */

#include "controller.hpp"
#include "model.hpp"
#include "view.hpp"

#include <gtk/gtk.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <windows.h>

// ── Construction ──────────────────────────────────────────────────────────────

FileManagerController::FileManagerController(FileSystemModel *model,
                                             FileManagerView *view)
    : m_model(model), m_view(view)
{
}

// ── Input handlers ────────────────────────────────────────────────────────────

void FileManagerController::onItemActivated(GtkTreePath *path)
{
    GtkTreeModel *treeModel =
        gtk_tree_view_get_model(GTK_TREE_VIEW(m_view->treeview()));
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(treeModel, &iter, path))
        return;

    if (m_view->isDriveView())
    {
        // Column 2 holds the drive root path (e.g. "C:\\").
        gchar *drivePath = nullptr;
        gtk_tree_model_get(treeModel, &iter, 2, &drivePath, -1);
        if (drivePath)
        {
            m_model->openPath(drivePath, false); // drives don't go onto the back stack
            g_free(drivePath);
        }
    }
    else
    {
        // Column 2 holds the full absolute path.
        gchar *fullPath = nullptr;
        gtk_tree_model_get(treeModel, &iter, 2, &fullPath, -1);
        if (!fullPath)
            return;

        struct stat st;
        if (stat(fullPath, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                m_model->openPath(fullPath);
            }
            else
            {
                // Open file with its default application — pure UI action.
                ShellExecute(nullptr, "open", fullPath,
                             nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
        g_free(fullPath);
    }
}

void FileManagerController::onBack()
{
    m_model->goToParent();

    // Update forward button state in the View.
    // (The Model notifies the View via onDirectory/onDrives; we only need
    //  to update the forward button, which isn't part of those callbacks.)
    gtk_widget_set_opacity(
        // Access through view's public accessor; forward button opacity.
        // We expose a helper via the model's canGoForward().
        m_view->treeview(), 1.0); // treeview itself doesn't change — placeholder
    // The View is updated by the Model's observer notifications.
}

void FileManagerController::onForward()
{
    m_model->goForward();
}

void FileManagerController::onNewFolder()
{
    std::string name = promptFolderName();
    if (name == "\x01")
        return; // User cancelled – just exit.

    bool ok = m_model->createFolder(name);
    if (!ok && !name.empty())
    {
        // Show OS error.
        GtkWidget *err = gtk_message_dialog_new(
            GTK_WINDOW(m_view->window()), GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Could not create folder \"%s\".", name.c_str());
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
    }
}

void FileManagerController::onPaste()
{
    m_model->paste();
}

void FileManagerController::onSearch()
{
    const char *query =
        gtk_entry_get_text(GTK_ENTRY(m_view->searchEntry()));
    if (!query || query[0] == '\0')
        return;

    m_model->search(query);
}

void FileManagerController::onRename(GtkTreePath *path)
{
    std::string oldName = m_view->nameAtPath(path);
    if (oldName.empty())
        return;

    while (true)
    {
        std::string newName = promptRename(oldName);
        if (newName.empty())
            break; // Cancelled.

        if (m_model->renameEntry(oldName, newName))
            break;

        // Show error and let user try again.
        GtkWidget *alert = gtk_message_dialog_new(
            GTK_WINDOW(m_view->window()), GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Could not rename \"%s\" to \"%s\".",
            oldName.c_str(), newName.c_str());
        gtk_dialog_run(GTK_DIALOG(alert));
        gtk_widget_destroy(alert);
    }
}

void FileManagerController::onCopy(GtkTreePath *path)
{
    std::string name = m_view->nameAtPath(path);
    if (name.empty())
        return;

    char cwd[MAX_PATH];
    getcwd(cwd, sizeof(cwd));
    m_model->markCopy(cwd, name);
}

void FileManagerController::onCut(GtkTreePath *path)
{
    std::string name = m_view->nameAtPath(path);
    if (name.empty())
        return;

    char cwd[MAX_PATH];
    getcwd(cwd, sizeof(cwd));
    m_model->markCut(cwd, name);
}

void FileManagerController::onDelete(GtkTreePath *path)
{
    std::string name = m_view->nameAtPath(path);
    if (name.empty())
        return;

    if (!confirmDelete(name))
        return;

    m_model->deleteEntry(name);
}

// ── Private dialog helpers ────────────────────────────────────────────────────

std::string FileManagerController::promptRename(const std::string &oldName)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Rename", GTK_WINDOW(m_view->window()), GTK_DIALOG_MODAL,
        "Ok", GTK_RESPONSE_OK,
        "Cancel", GTK_RESPONSE_CANCEL,
        nullptr);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), oldName.c_str());
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a new name");
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(area), entry);
    gtk_widget_show_all(dialog);

    std::string result;
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text && text[0] != '\0')
            result = text;
    }

    gtk_widget_destroy(dialog);
    return result;
}

std::string FileManagerController::promptFolderName()
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "New Folder", GTK_WINDOW(m_view->window()), GTK_DIALOG_MODAL,
        "Ok", GTK_RESPONSE_OK,
        "Cancel", GTK_RESPONSE_CANCEL,
        nullptr);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a folder name");
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(area), entry);
    gtk_widget_show_all(dialog);

    std::string result;
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        result = text ? text : "";
    }
    else
    {
        result = "\x01"; // Sentinel: user cancelled — don't create anything.
    }

    gtk_widget_destroy(dialog);

    // Return "" for auto-name, non-empty for a chosen name, "\x01" for cancel.
    if (result == "\x01")
        return "\x01";
    return result;
}

bool FileManagerController::confirmDelete(const std::string &name)
{
    GtkWidget *confirm = gtk_message_dialog_new(
        GTK_WINDOW(m_view->window()), GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Are you sure you want to delete '%s'?", name.c_str());

    gint response = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);
    return response == GTK_RESPONSE_YES;
}
