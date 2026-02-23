/**
 * @file icon.c
 * @brief Implementation of the file-icon helper.
 */

#include "icon.h"

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>

/* ── File icon helper ───────────────────────────────────────────────────── */

/**
 * @brief Retrieves a scaled GdkPixbuf icon for the given file path.
 *
 * @param file_path Path to the file whose icon should be loaded.
 * @param size      Desired icon size in pixels (width and height).
 * @return GdkPixbuf* Caller-owned pixbuf, or NULL on failure.
 */
GdkPixbuf *get_file_icon(const char *file_path, int size)
{
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    GFile        *file  = g_file_new_for_path(file_path);
    GFileInfo    *info  = g_file_query_info(
        file,
        G_FILE_ATTRIBUTE_STANDARD_ICON "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        G_FILE_QUERY_INFO_NONE,
        NULL,
        NULL);

    if (!info)
    {
        g_object_unref(file);
        return NULL;
    }

    GIcon       *icon      = g_file_info_get_icon(info);
    GtkIconInfo *icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, 0);
    GdkPixbuf   *pixbuf    = NULL;

    if (!icon_info)
    {
        /* Fall back to generic media icons based on file extension. */
        const char *ext = strrchr(file_path, '.');
        if (ext)
        {
            if (g_strcmp0(ext, ".mp3") == 0 || g_strcmp0(ext, ".wav") == 0)
                pixbuf = gtk_icon_theme_load_icon(theme, "audio-x-generic",
                                                  size, 0, NULL);
            else if (g_strcmp0(ext, ".mp4") == 0 || g_strcmp0(ext, ".avi") == 0)
                pixbuf = gtk_icon_theme_load_icon(theme, "video-x-generic",
                                                  size, 0, NULL);
        }
    }
    else
    {
        GError *error = NULL;
        pixbuf = gtk_icon_info_load_icon(icon_info, &error);
        if (!pixbuf && error)
            g_error_free(error);
        g_object_unref(icon_info);
    }

    /* Scale to the requested size if the theme returned a different size. */
    if (pixbuf &&
        (gdk_pixbuf_get_width(pixbuf)  != size ||
         gdk_pixbuf_get_height(pixbuf) != size))
    {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, size, size,
                                                     GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
        pixbuf = scaled;
    }

    g_object_unref(info);
    g_object_unref(file);
    return pixbuf;
}
