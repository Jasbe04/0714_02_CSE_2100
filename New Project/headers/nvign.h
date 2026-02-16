#ifndef NVIGN_H
#define NVIGN_H
#include "app_data.h"
#include "icon.h"

void show_drives(Data *data);
void open_directory(Data *data);
void go_to_parent_folder(GtkWidget *widget, GdkEventButton *event, gpointer data);
void go_to_previous_folder(GtkWidget *widget, GdkEventButton *event, gpointer data);

#endif