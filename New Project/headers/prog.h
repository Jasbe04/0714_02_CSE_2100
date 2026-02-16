#ifndef PROG_H
#define PROG_H
#include "app_data.h"

void update_progress_bar(CopyProgress *progress);
long long calculate_total_size(const char *path);
gboolean on_progress_close(GtkWidget *widget, GdkEvent *event, gpointer data);

#endif