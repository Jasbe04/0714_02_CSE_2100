/**
 * @file folder_creator.cpp
 * @brief SRP: new-folder dialog and mkdir — nothing else.
 */

#include "folder_creator.hpp"

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <glib.h>

bool FolderCreator::createFolder(GtkWidget *parentWindow)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "New Folder", GTK_WINDOW(parentWindow), GTK_DIALOG_MODAL,
        "Ok",     GTK_RESPONSE_OK,
        "Cancel", GTK_RESPONSE_CANCEL,
        nullptr);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a folder name");
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(area), entry);

    bool created = false;
    while (true)
    {
        gtk_widget_show_all(dialog);
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response != GTK_RESPONSE_OK)
            break;

        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (tryCreate(parentWindow, text))
        {
            created = true;
            break;
        }
        // tryCreate shows an error dialog if the name is taken; loop again.
    }

    gtk_widget_destroy(dialog);
    return created;
}

bool FolderCreator::tryCreate(GtkWidget *parentWindow, const char *name)
{
    if (name[0] == '\0')
    {
        // Empty name: use "New Folder" with auto-numbering on collision.
        if (mkdir("New Folder") == 0)
            return true;

        for (int i = 1; i < 1000; ++i)
        {
            gchar *candidate = g_strdup_printf("New folder (%d)", i);
            bool ok = (mkdir(candidate) == 0);
            g_free(candidate);
            if (ok) return true;
        }
        return false;
    }

    if (mkdir(name) != 0)
    {
        // Show the OS error message and let the caller loop.
        GtkWidget *alert = gtk_message_dialog_new(
            GTK_WINDOW(parentWindow), GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "%s", strerror(errno));
        gtk_dialog_run(GTK_DIALOG(alert));
        gtk_widget_destroy(alert);
        return false;
    }
    return true;
}
