/**
 * @file icon.hpp
 * @brief SRP — Single Responsibility: file icon loading only.
 *
 * Wraps get_file_icon() from the original icon.c into a class with one job:
 * return the appropriate GdkPixbuf for a given file path.
 *
 * SOLID principles demonstrated here:
 *   S (SRP) — One class, one job: load icons.
 *   O (OCP) — New icon strategies (e.g. thumbnail previews) can extend this
 *              without touching NavigationManager or SearchService.
 */

#ifndef ICON_LOADER_HPP
#define ICON_LOADER_HPP

#include <gtk/gtk.h>

/**
 * @brief Retrieves GTK icons for files and folders.
 */
class IconLoader
{
public:
    /**
     * @brief Returns a @p size × @p size pixbuf for the given file path.
     *        Caller owns the returned pixbuf (must g_object_unref).
     * @param filePath Path to the file whose icon is needed.
     * @param size     Desired pixel size (width and height).
     * @return GdkPixbuf* or nullptr on failure.
     */
    GdkPixbuf *getFileIcon(const char *filePath, int size) const;

    /**
     * @brief Returns a @p size × @p size pixbuf scaled from @p source.
     *        Caller owns the returned pixbuf.
     */
    static GdkPixbuf *scale(GdkPixbuf *source, int size);
};

#endif // ICON_LOADER_HPP
