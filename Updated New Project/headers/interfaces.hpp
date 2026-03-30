/**
 * @file interfaces.hpp
 * @brief ISP — Interface Segregation Principle
 *
 * Instead of passing the giant Data struct to every function, we define small
 * focused interfaces.  Each module only depends on the interface it actually needs.
 *
 * SOLID principles demonstrated here:
 *   I (ISP) — Three small interfaces replace the monolithic Data* dependency.
 *   D (DIP) — High-level modules depend on these abstractions, not on concrete types.
 */

#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include <gtk/gtk.h>

// ── INavigable ───────────────────────────────────────────────────────────────
/**
 * @brief Abstract interface for anything that can navigate directories.
 *
 * NavigationManager implements this; UIController depends on it.
 * ISP: search and clipboard modules do NOT need this interface.
 */
class INavigable
{
public:
    virtual void openDirectory()      = 0;
    virtual void showDrives()         = 0;
    virtual void goToParent()         = 0;
    virtual void goForward()          = 0;
    virtual ~INavigable() {}
};

// ── IClipboard ───────────────────────────────────────────────────────────────
/**
 * @brief Abstract interface for clipboard (copy/cut/paste) operations.
 *
 * ISP: navigation and search modules do NOT need this interface.
 */
class IClipboard
{
public:
    virtual void markCopy(const char *dir, const char *name) = 0;
    virtual void markCut (const char *dir, const char *name) = 0;
    virtual void paste()                                     = 0;
    virtual bool hasPending() const                          = 0;
    virtual ~IClipboard() {}
};

// ── ISearchable ──────────────────────────────────────────────────────────────
/**
 * @brief Abstract interface for file search capability.
 *
 * ISP: navigation and clipboard modules do NOT need this interface.
 */
class ISearchable
{
public:
    virtual void search(const char *query, const char *basePath) = 0;
    virtual ~ISearchable() {}
};

// ── IProgressReporter ────────────────────────────────────────────────────────
/**
 * @brief Abstract interface for reporting progress of long-running operations.
 *
 * DIP: FileCopier and FileDeleter depend on this abstraction,
 *      not on the concrete GTK progress-bar implementation.
 */
class IProgressReporter
{
public:
    virtual void update(double fraction)  = 0;
    virtual void show(const char *title)  = 0;
    virtual void hide()                   = 0;
    virtual ~IProgressReporter() {}
};

#endif // INTERFACES_HPP
