/**
 * @file main.c
 * @brief Application entry point for the GTK file manager.
 *
 * Initialises GTK, allocates and zeroes the central Data struct, then
 * delegates all UI construction and event-loop management to set_ui().
 */

#include <gtk/gtk.h>
#include "app_data.h"
#include "ui.h"

/**
 * @brief Application entry point.
 *
 * Initialises the GTK toolkit, allocates the Data struct with all fields
 * zeroed, and calls set_ui() which builds the window and starts the GTK
 * main event loop.  Frees the Data struct on clean exit.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return int Returns 0 on clean exit.
 */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    Data *data = g_new0(Data, 1);

    /* All integer / pointer fields are zeroed by g_new0; set explicit defaults. */
    data->depth       = 0;
    data->nav_forward = 0;
    data->is_copy     = 0;
    data->is_cut      = 0;
    data->fraction    = 0.1;
    data->head_next   = NULL;
    data->new_node_next = NULL;
    data->head_back   = NULL;
    data->new_node_back = NULL;

    set_ui(data);

    g_free(data);
    return 0;
}
