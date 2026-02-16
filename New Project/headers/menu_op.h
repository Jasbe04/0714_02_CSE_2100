#ifndef MENU_OP
#define MENU_OP
#include "nvign.h"
#include "prog.h"

void folder_rename(GtkMenuItem *item, gpointer data);
void delete_function(const gchar *path, CopyProgress *progress);
void delete(GtkMenuItem *item, gpointer data);
void copy(GtkMenuItem *item, gpointer data);
void cut(GtkMenuItem *item, gpointer data);

#endif