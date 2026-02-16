#include "icon.h"

/*
 * Returns the icon for a given file as a GdkPixbuf.
 * - Uses the system GtkIconTheme to fetch the standard file icon.
 * - Falls back to generic audio/video/text icons if the file type is unknown.
 * - Scales the icon to the requested size.
 * 
 * Parameters:
 *   filePath - path to the file
 *   size     - desired width and height of the icon
 * 
 * Returns:
 *   A GdkPixbuf pointer representing the icon.
 *   The caller is responsible for freeing the pixbuf when done.
 * 
 * Notes:
 *   - Handles both uppercase and lowercase file extensions.
 *   - Returns a default "text-x-generic" icon if no theme icon is available.
 */


GdkPixbuf *get_file_icon(const char *filePath, int size)
{
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    GFile *file = g_file_new_for_path(filePath);
    GFileInfo *info = g_file_query_info(
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

    GIcon *icon = g_file_info_get_icon(info);
    GtkIconInfo *iconInfo = gtk_icon_theme_lookup_by_gicon(theme, icon, size, 0);

    GdkPixbuf *pixbuf = NULL;

    if (!iconInfo)
    {

        const char *ext = strrchr(filePath, '.');
        if (ext)
        {
            if (g_strcmp0(ext, ".mp3") == 0 || g_strcmp0(ext, ".wav") == 0)
            {
                pixbuf = gtk_icon_theme_load_icon(theme, "audio-x-generic", size, 0, NULL);
            }
            else if (g_strcmp0(ext, ".mp4") == 0 || g_strcmp0(ext, ".avi") == 0)
            {
                pixbuf = gtk_icon_theme_load_icon(theme, "video-x-generic", size, 0, NULL);
            }
        }
    }
    else
    {
        GError *error = NULL;
        pixbuf = gtk_icon_info_load_icon(iconInfo, &error);
        if (!pixbuf && error)
        {
            g_error_free(error);
        }
        g_object_unref(iconInfo);
    }

    if (pixbuf && (gdk_pixbuf_get_width(pixbuf) != size || gdk_pixbuf_get_height(pixbuf) != size))
    {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, size, size, GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
        pixbuf = scaled;
    }

    g_object_unref(info);
    g_object_unref(file);

    return pixbuf;
}
