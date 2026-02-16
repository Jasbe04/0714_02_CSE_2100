#ifndef UI_H
#define UI_H
#include "file_op.h"
#include "search.h"

void select_directory(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);
gboolean right_button_click(GtkTreeView *treeview, GdkEventButton *event, gpointer data);
void set_ui(Data *data);

#endif