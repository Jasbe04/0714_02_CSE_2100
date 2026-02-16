#include "nvign.h"

/*
 * Displays all available drives on the system.
 * - Hides search box and disables buttons not applicable at drive level.
 * - Sets the label to "Devices and Drives" and styles it with CSS.
 * - Iterates through all logical drives (A-Z) and retrieves their volume names.
 * - Adds each drive as an entry in the GtkListStore with an icon.
 *
 * Parameters:
 *   data - pointer to the main Data structure (app state)
 */

void show_drives(Data *data)
{
    Data *appData = data;
    gtk_widget_hide(appData->boxSearch);
    gtk_widget_set_opacity(appData->backEvent, 0.1);
    gtk_widget_set_opacity(appData->newFolderEvent, 0.1);
    gtk_widget_set_opacity(appData->pasteEvent, 0.1);
    if (gtk_widget_get_parent(appData->boxSearch))
        gtk_widget_hide(appData->boxSearch);
    gtk_label_set_text(GTK_LABEL(appData->labelPath), "Devices and Drives");
    gtk_widget_set_name(appData->labelPath, "labelPath");
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,"#labelPath { font-size: 20px; font-family: Arial; font-weight: bold; }",-1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    GtkTreeIter iter;
    DWORD drives = GetLogicalDrives();
    gchar drive;
    for (drive = 'A'; drive <= 'Z'; drive++)
    {
        if (drives & (1 << (drive - 'A')))
        {
            gchar path[MAX_PATH + 1] = "";
            gchar name1[MAX_PATH + 1] = "";
            gchar name2[MAX_PATH + 1] = "";
            gchar realName[MAX_PATH + 1] = "";
            path[0] = drive;
            name2[0] = drive;
            strcat(path, ":\\");
            strcat(name2, ":");
            GetVolumeInformation(path, name1, sizeof(name1), NULL, NULL, NULL, NULL, 0);
            if (name1 != "")
            {
                strcat(name1, " ");
            }
            sprintf(realName, "%s(%s)", name1, name2);
            gtk_list_store_append(appData->store, &iter);
            gtk_list_store_set(appData->store, &iter, 0, appData->scaledPixbuf4, 1, realName, -1);
        }
    }
}

/*
 * Opens the current directory and displays its contents in the file manager.
 * - Shows the search box and enables/disables buttons based on copy/cut state.
 * - Clears previous entries from the list store.
 * - Reads all non-hidden files and directories from the current working directory.
 * - For directories, uses a default folder icon; for files, attempts to load a proper icon.
 * - Updates the scrolled window with either the list of items or a "This folder is empty" label.
 *
 * Parameters:
 *   data - pointer to the main Data structure (app state)
 */

void open_directory(Data *data)
{
  Data *appData = data;
  gtk_widget_show(appData->boxSearch);
  if (appData->isCopyMode == 1 || appData->isCutMode == 1)
  {
    gtk_widget_set_opacity(appData->pasteEvent, 1);
  }
  if (appData->forwardCount == 0)
  {
    gtk_widget_set_opacity(appData->nextEvent, 0.1);
  }
  gtk_widget_set_opacity(appData->backEvent, 1);
  gtk_widget_set_opacity(appData->newFolderEvent, 1);
  GtkTreeIter iter;
  gtk_list_store_clear(appData->store);
  DIR *dp = opendir(".");
  gchar path[MAX_PATH + 1];
  getcwd(path, sizeof(path));
  gchar realPath[MAX_PATH + 50];
  sprintf(realPath, "Current path: %s", path);
  gtk_label_set_text(GTK_LABEL(appData->labelPath), realPath);
  gtk_widget_show_all(appData->parentBox);
  int h = 0;
  struct dirent *entry;
  while ((entry = readdir(dp)) != NULL)
  {
    DWORD att = GetFileAttributes(entry->d_name);
    if (att == INVALID_FILE_ATTRIBUTES)
    {
      continue;
    }
    if (att & FILE_ATTRIBUTE_HIDDEN || att & FILE_ATTRIBUTE_SYSTEM)
    {
      continue;
    }
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
    {
      h++;
      GError *error = NULL;
      char fullPath[MAX_PATH] = "";
      snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, entry->d_name);
      struct stat fstat;
      stat(fullPath, &fstat);
      GdkPixbuf *scaledPixbuf;
      if (S_ISDIR(fstat.st_mode))
      {
        scaledPixbuf = gdk_pixbuf_scale_simple(appData->iconPixbuf, 48, 48, GDK_INTERP_BILINEAR);
      }
      else
      {
        scaledPixbuf = get_file_icon(entry->d_name, 48);
        if (!scaledPixbuf)
        {
          scaledPixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                                  "text-x-generic", 48, 0, NULL);
        }
      }
      gtk_list_store_append(appData->store, &iter);
      gtk_list_store_set(appData->store, &iter, 0, scaledPixbuf, 1, entry->d_name, 2, fullPath, -1);
      if (scaledPixbuf)
        g_object_unref(scaledPixbuf);
    }
  }
  GtkWidget *child = gtk_bin_get_child(GTK_BIN(appData->scrolledWindow));
  if (child)
  {
    gtk_container_remove(GTK_CONTAINER(appData->scrolledWindow), child);
  }

  if (h == 0)
  {
    GtkWidget *emptyLabel = gtk_label_new("This folder is empty");
    gtk_container_add(GTK_CONTAINER(appData->scrolledWindow), emptyLabel);
  }
  else
  {
    gtk_container_add(GTK_CONTAINER(appData->scrolledWindow), appData->treeview);
  }

  gtk_widget_show_all(appData->scrolledWindow);
  closedir(dp);
}

/*
 * Navigates to the parent folder of the current directory.
 * - Updates depth level and forward navigation count.
 * - Saves the current path in the forward history linked list.
 * - If the new depth level is 0, shows drives instead of folder contents.
 * - Otherwise, updates the current working directory and refreshes the file list.
 *
 * Parameters:
 *   widget - the GtkWidget that triggered this callback
 *   event  - the GdkEventButton that triggered this callback
 *   data   - pointer to the main Data structure (app state)
 */

void go_to_parent_folder(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  Data *appData = (Data *)data;
  gtk_widget_set_opacity(appData->nextEvent, 1);
  if (appData->depthLevel != 0)
  {
    appData->depthLevel--;
    appData->forwardCount++;
    gchar currentPath[MAX_PATH + 1] = "";
    getcwd(currentPath, sizeof(currentPath));
    appData->newNode1 = (NextPath *)malloc(sizeof(NextPath));
    strcpy(appData->newNode1->path, currentPath);
    appData->newNode1->next = NULL;
    if (appData->head1 == NULL)
    {
      appData->head1 = appData->newNode1;
    }
    else
    {
      appData->newNode1->next = appData->head1;
      appData->head1 = appData->newNode1;
    }
    if (appData->depthLevel == 0)
    {
      gtk_list_store_clear(appData->store);
      show_drives(appData);
    }
    else
    {
      chdir(appData->head2->path);
      BackPath *dd = appData->head2;
      appData->head2 = appData->head2->next;
      free(dd);
      open_directory(appData);
    }
  }
}

/*
 * Navigates to the previously visited folder (forward navigation).
 * - Updates forward/backward navigation counters.
 * - Saves the current path in the backward history linked list.
 * - Retrieves the next path from forward history, changes directory, and refreshes file list.
 *
 * Parameters:
 *   widget - the GtkWidget that triggered this callback
 *   event  - the GdkEventButton that triggered this callback
 *   data   - pointer to the main Data structure (app state)
 */

void go_to_previous_folder(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  Data *appData = (Data *)data;
  if (appData->forwardCount != 0)
  {
    appData->forwardCount--;
    appData->depthLevel++;
    appData->newNode2 = (BackPath *)malloc(sizeof(BackPath));
    char currentPath[MAX_PATH] = "";
    getcwd(currentPath, sizeof(currentPath));
    strcpy(appData->newNode2->path, currentPath);
    appData->newNode2->next = NULL;
    if (appData->head2 == NULL)
    {
      appData->head2 = appData->newNode2;
    }
    else
    {
      appData->newNode2->next = appData->head2;
      appData->head2 = appData->newNode2;
    }
    char folderName[MAX_PATH + 1];
    strcpy(folderName, appData->head1->path);
    NextPath *dd = appData->head1;
    appData->head1 = appData->head1->next;
    free(dd);
    chdir(folderName);
    open_directory(appData);
  }
}

