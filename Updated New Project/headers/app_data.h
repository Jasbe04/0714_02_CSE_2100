#ifndef APP_DATA_H
#define APP_DATA_H

#include <gtk/gtk.h>
#include <windows.h>

/* ── Linked-list node for forward navigation history ────────────────────── */
/**
 * @brief Singly-linked list node that stores one directory path for the
 *        forward-navigation stack.
 */
typedef struct NextPath
{
    char path[MAX_PATH + 1]; /**< Absolute path stored in this node.       */
    struct NextPath *next;   /**< Pointer to the next node, or NULL.        */
} NextPath;

/* ── Linked-list node for backward navigation history ───────────────────── */
/**
 * @brief Singly-linked list node that stores one directory path for the
 *        backward-navigation stack.
 */
typedef struct BackPath
{
    char path[MAX_PATH + 1]; /**< Absolute path stored in this node.       */
    struct BackPath *next;   /**< Pointer to the next node, or NULL.        */
} BackPath;

/* ── Progress tracking for copy / move / delete operations ─────────────── */
/**
 * @brief Holds all state needed to update a GTK progress bar during a
 *        file operation (copy, move, or delete).
 */
typedef struct
{
    GtkWidget *progress_bar; /**< The GtkProgressBar widget to update.      */
    gdouble    fraction;     /**< Current progress expressed as 0.0–1.0.    */
    gdouble    total_bytes;  /**< Total byte count of the operation target.  */
    gdouble    copied_bytes; /**< Bytes processed so far.                    */
} CopyProgress;

/* ── Central application state ──────────────────────────────────────────── */
/**
 * @brief Aggregates all UI widgets, pixbufs, and navigation state for the
 *        file manager.  A single heap-allocated instance is passed by pointer
 *        to every callback instead of using global variables.
 */
typedef struct
{
    /* ── Pixbufs ─────────────────────────────────────────────────────────── */
    GdkPixbuf *icon_pixbuf;    /**< Default folder icon pixbuf.             */
    GdkPixbuf *scaled_pixbuf4; /**< Scaled drive icon (40×40).              */
    GdkPixbuf *pixbuf1;        /**< Raw back-button pixbuf.                 */
    GdkPixbuf *pixbuf2;        /**< Raw next-button pixbuf.                 */

    /* ── Top-level window and core containers ────────────────────────────── */
    GtkWidget    *window;          /**< Main application window.            */
    GtkListStore *store;           /**< List store backing the tree view.   */
    GtkWidget    *treeview;        /**< Tree view displaying directory entries. */
    GtkWidget    *parent_box;      /**< Vertical box containing all widgets.*/
    GtkWidget    *label_status;    /**< Status label ("This folder is empty"). */
    GtkWidget    *label_path;      /**< Label showing the current path.     */
    GtkWidget    *box_search;      /**< Horizontal box holding search widgets.*/
    GtkWidget    *box_path;        /**< Horizontal box holding the path label.*/
    GtkWidget    *box;             /**< Horizontal box holding toolbar buttons.*/
    GtkWidget    *full_box;        /**< Horizontal box containing toolbar + search.*/
    GtkWidget    *image1;          /**< Back-button image widget.           */
    GtkWidget    *image2;          /**< Next-button image widget.           */
    GtkWidget    *scrolled_window; /**< Scrolled container for the tree view.*/
    GtkWidget    *search_entry;    /**< Text entry for search queries.      */
    GtkWidget    *search_button;   /**< Button that triggers a search.      */

    /* ── Toolbar event boxes ─────────────────────────────────────────────── */
    GtkWidget *event1; /**< Event box wrapping the Back button.             */
    GtkWidget *event2; /**< Event box wrapping the Next button.             */
    GtkWidget *event3; /**< Event box wrapping the New-Folder button.       */
    GtkWidget *event4; /**< Event box wrapping the Paste button.            */

    /* ── Clipboard / navigation state ───────────────────────────────────── */
    gchar   pathway1[MAX_PATH + 1]; /**< Directory containing the copied/cut item. */
    gchar   fname1[MAX_PATH + 1];   /**< Name of the copied/cut item.              */
    int     depth;       /**< Current directory depth (0 = drive list view). */
    int     nav_forward; /**< Number of directories available to revisit forward. */
    int     is_copy;     /**< Non-zero when an item has been marked for copy.*/
    int     is_cut;      /**< Non-zero when an item has been marked for cut. */
    gdouble fraction;    /**< Last recorded progress fraction (0.0–1.0).    */

    /* ── Navigation stacks ───────────────────────────────────────────────── */
    NextPath *head_next;     /**< Head of the forward-navigation stack.     */
    NextPath *new_node_next; /**< Scratch pointer for new forward nodes.    */
    BackPath *head_back;     /**< Head of the backward-navigation stack.    */
    BackPath *new_node_back; /**< Scratch pointer for new backward nodes.   */
} Data;

#endif /* APP_DATA_H */
