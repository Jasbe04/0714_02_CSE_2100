#ifndef ICON_H
#define ICON_H

#include <gtk/gtk.h>

/**
 * @brief Retrieves a scaled GdkPixbuf icon for the given file path.
 *
 * Queries GIO for the file's associated GIcon via the icon theme.  If the
 * theme lookup fails, falls back to built-in audio or video generic icons
 * based on the file extension.  The returned pixbuf is scaled to exactly
 * @p size × @p size pixels when the theme icon has a different native size.
 *
 * @param file_path Path to the file whose icon should be loaded.
 * @param size      Desired icon size in pixels (width and height).
 * @return GdkPixbuf* Caller-owned pixbuf, or NULL if no icon could be loaded.
 *                    The caller must call g_object_unref() when done.
 */
GdkPixbuf *get_file_icon(const char *file_path, int size);

#endif /* ICON_H */
