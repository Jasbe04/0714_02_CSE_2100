/**
 * @file folder_creator.hpp
 * @brief SRP — Single Responsibility: new-folder dialog only.
 *
 * In the original code, create_new_folder() in file_op.c mixed UI dialog logic
 * with file-system mkdir calls inside the same function.
 *
 * FolderCreator has one job: prompt the user for a name and create the folder.
 * It does not touch navigation, progress bars, or clipboard state.
 *
 * SOLID principles demonstrated here:
 *   S (SRP) — One class, one job: create a folder via a dialog.
 *   D (DIP) — UIController calls this through its pointer; could be swapped
 *              for a headless version in tests.
 */

#ifndef FOLDER_CREATOR_HPP
#define FOLDER_CREATOR_HPP

#include <gtk/gtk.h>

/**
 * @brief Shows a dialog asking for a folder name and creates it in the CWD.
 */
class FolderCreator
{
public:
    /**
     * @brief Shows the "New Folder" dialog and creates the folder.
     * @param parentWindow GTK parent window for the dialog.
     * @return true if a folder was successfully created, false if cancelled.
     */
    bool createFolder(GtkWidget *parentWindow);

private:
    /**
     * @brief Tries to create a folder with the given name.
     *        If @p name is empty, uses "New Folder" with auto-numbering.
     * @param parentWindow Parent for any error dialogs.
     * @param name         Desired folder name (may be empty).
     * @return true on success.
     */
    bool tryCreate(GtkWidget *parentWindow, const char *name);
};

#endif // FOLDER_CREATOR_HPP
