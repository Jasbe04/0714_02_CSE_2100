/**
 * @file icon.cpp
 * @brief SRP: load and scale file icons — nothing else.
 */

#include "icon.hpp"
#include <gio/gio.h>
#include <string.h>

GdkPixbuf *IconLoader::getFileIcon(const char *filePath, int size) const
{
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    GFile        *file  = g_file_new_for_path(filePath);
    GFileInfo    *info  = g_file_query_info(
        file,
        G_FILE_ATTRIBUTE_STANDARD_ICON "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        G_FILE_QUERY_INFO_NONE,
        nullptr, nullptr);

    if (!info)
    {
        g_object_unref(file);
        return nullptr;
    }

    GIcon       *icon      = g_file_info_get_icon(info);
    GtkIconInfo *icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, (GtkIconLookupFlags)0);
    GdkPixbuf   *pixbuf    = nullptr;

    if (!icon_info)
    {
        // Fall back to generic media icons based on extension.
        const char *ext = strrchr(filePath, '.');
        if (ext)
        {
            if (g_strcmp0(ext, ".mp3") == 0 || g_strcmp0(ext, ".wav") == 0)
                pixbuf = gtk_icon_theme_load_icon(theme, "audio-x-generic", size, (GtkIconLookupFlags)0, nullptr);
            else if (g_strcmp0(ext, ".mp4") == 0 || g_strcmp0(ext, ".avi") == 0)
                pixbuf = gtk_icon_theme_load_icon(theme, "video-x-generic", size, (GtkIconLookupFlags)0, nullptr);
        }
    }
    else
    {
        pixbuf = gtk_icon_info_load_icon(icon_info, nullptr);
        g_object_unref(icon_info);
    }

    // Resize if the theme returned a different size.
    if (pixbuf &&
        (gdk_pixbuf_get_width(pixbuf)  != size ||
         gdk_pixbuf_get_height(pixbuf) != size))
    {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, size, size, GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
        pixbuf = scaled;
    }

    g_object_unref(info);
    g_object_unref(file);
    return pixbuf;
}

/*static*/
GdkPixbuf *IconLoader::scale(GdkPixbuf *source, int size)
{
    if (!source) return nullptr;
    return gdk_pixbuf_scale_simple(source, size, size, GDK_INTERP_BILINEAR);
}
