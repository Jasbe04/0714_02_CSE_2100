#ifndef NVIGN_H
#define NVIGN_H

#include <gtk/gtk.h>
#include "app_data.h"

/**
 * @brief Populates the file list with the available logical drives on the system.
 *
 * Resets toolbar button opacities, hides the search bar, updates the path
 * label, and enumerates all Windows drive letters via GetLogicalDrives(),
 * adding each drive to the list store with its volume label and letter.
 *
 * @param data Pointer to the Data struct holding all UI and navigation state.
 */
void show_drives(Data *data);

/**
 * @brief Populates the file list with the contents of the current working directory.
 *
 * Clears the store, reads the current directory with opendir(), resolves
 * icons for each visible entry, and rebuilds the scrolled-window content.
 * Hidden and system files are skipped.
 *
 * @param data Pointer to the Data struct holding all UI and navigation state.
 */
void open_directory(Data *data);

/**
 * @brief GTK signal handler that navigates one level up in the directory hierarchy.
 *
 * Decrements the depth counter, pushes the current path onto the forward
 * stack, then either shows the drive list (at depth 0) or pops the backward
 * stack and calls open_directory().
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void go_to_parent_folder(GtkWidget *widget, GdkEventButton *event,
                         gpointer data);

/**
 * @brief GTK signal handler that navigates forward to a previously visited directory.
 *
 * Decrements the forward counter, pushes the current path onto the backward
 * stack, pops the forward stack, and calls open_directory().
 *
 * @param widget The event box widget that received the button press (unused).
 * @param event  The GDK button event (unused).
 * @param data   Pointer to the Data struct cast to gpointer.
 */
void go_to_previous_folder(GtkWidget *widget, GdkEventButton *event,
                            gpointer data);

#endif /* NVIGN_H */
