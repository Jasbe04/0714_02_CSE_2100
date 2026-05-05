/**
 * @file interfaces.hpp
 * @brief ISP / DIP — small focused abstractions shared across MVC layers.
 *
 * In the MVC model these interfaces serve two purposes:
 *
 *  1.  IProgressReporter — used by the Model's file-operation layer to report
 *      progress without depending on GTK.  The View provides a concrete
 *      implementation.
 *
 *  2.  IModelObserver — callback contract the View implements so the Model
 *      can push state changes without knowing anything about GTK widgets.
 *      (Defined in model.hpp to avoid a circular include.)
 *
 * SOLID principles:
 *   I (ISP) — each interface is minimal and focused.
 *   D (DIP) — Model and Controller depend on these abstractions, not on
 *              concrete GTK classes.
 */

#ifndef INTERFACES_HPP
#define INTERFACES_HPP

// ── IProgressReporter ────────────────────────────────────────────────────────
/**
 * @brief Abstract interface for reporting progress of long-running file ops.
 *
 * The Model's file-operation classes (CopyOperation, DeleteOperation …) call
 * this.  The concrete GTK implementation lives in the View layer (prog.hpp).
 */
class IProgressReporter
{
public:
    virtual void update(double fraction)  = 0;
    virtual void show(const char *title)  = 0;
    virtual void hide()                   = 0;
    virtual void showCalculating() = 0;      // new
    virtual void hideCalculating() = 0;
    virtual ~IProgressReporter() {}
};

#endif // INTERFACES_HPP
