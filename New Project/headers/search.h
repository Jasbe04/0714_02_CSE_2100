#ifndef SEARCH_H
#define SEARCH_H
#include "app_data.h"
#include "icon.h"

void search_files(const char *basePath, const char *query, GtkListStore *store, Data *data);
void on_search_clicked(GtkButton *button, gpointer data);

#endif