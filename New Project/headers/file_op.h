#ifndef FILE_OP_H
#define FILE_OP_H
#include "menu_op.h"

void create_new_folder(GtkWidget *widget, GdkEventButton *event, gpointer data);
void delete_function_for_cut(const char *path, CopyProgress *progress);
void paste(const gchar *src, const gchar *dest, CopyProgress *progress);
void copy_function(GtkWidget *widget,GdkEventButton *event,gpointer data);

#endif
