#ifndef APP_DATA_H
#define APP_DATA_H
#include <gtk/gtk.h>
#include <windows.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <gio/gio.h>
#include <string.h>
typedef struct nextPath
{
    char path[MAX_PATH + 1];
    struct nextPath *next;
} NextPath;
typedef struct backPath
{
    char path[MAX_PATH + 1];
    struct backPath *next;
} BackPath;
typedef struct
{
    GtkWidget *progressBar;
    gdouble fraction;
    gdouble totalBytes;
    gdouble copiedBytes;
} CopyProgress;
typedef struct Data
{
    GdkPixbuf *iconPixbuf;
    GdkPixbuf *scaledPixbuf4;
    GtkWidget *window;
    GtkListStore *store;
    GtkWidget *treeview;
    GtkWidget *parentBox;
    GtkWidget *labelStatus;
    GtkWidget *labelPath;
    GtkWidget *boxSearch;
    GtkWidget *boxPath;
    GtkWidget *box1;
    GtkWidget *box2;
    GtkWidget *box3;
    GtkWidget *backEvent;
    GtkWidget *nextEvent;
    GtkWidget *newFolderEvent;
    GtkWidget *pasteEvent;
    GtkWidget *box;
    GtkWidget *fullBox;
    GdkPixbuf *pixbuf1;
    GdkPixbuf *pixbuf2;
    GtkWidget *image1;
    GtkWidget *image2;
    GtkWidget *scrolledWindow;
    GtkWidget *searchEntry;
    GtkTreePath *path;
    GtkWidget *searchButton;
    gchar pathWay1[MAX_PATH + 1];
    gchar fName1[MAX_PATH + 1];
    CopyProgress *progressWindow;
    int forwardCount;
    int depthLevel;
    int isCopyMode;
    int isCutMode;
    NextPath *head1, *newNode1;
    BackPath *head2, *newNode2;
} Data;

#endif