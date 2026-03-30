/**
 * @file nvign.hpp
 * @brief SRP — Single Responsibility: directory navigation only.
 *        ISP — Implements INavigable; unrelated modules never see this class.
 *
 * In the original code, nvign.c mixed navigation logic with direct GTK widget
 * manipulation and was called from almost everywhere.
 *
 * NavigationManager owns all navigation state (back/forward stacks, depth) and
 * updates the UI through the AppState widgets it was given at construction.
 *
 * SOLID principles demonstrated here:
 *   S (SRP)  — Navigates directories.  Does not copy files or build the UI.
 *   I (ISP)  — Implements only INavigable; callers depend on the interface.
 *   D (DIP)  — UIController depends on INavigable*, not NavigationManager*.
 */

#ifndef NAVIGATION_MANAGER_HPP
#define NAVIGATION_MANAGER_HPP

#include "interfaces.hpp"
#include "app_state.hpp"
#include "icon.hpp"

/**
 * @brief Manages directory navigation: drives list, open, back, and forward.
 */
class NavigationManager : public INavigable
{
public:
    /**
     * @brief Constructs the manager.
     * @param state Shared application state (widgets + stacks).
     * @param icons IconLoader used to get per-file icons.
     */
    NavigationManager(AppState *state, IconLoader *icons);

    // ── INavigable ────────────────────────────────────────────────────────────
    void openDirectory() override;
    void showDrives()    override;
    void goToParent()    override;
    void goForward()     override;

private:
    AppState   *m_state;
    IconLoader *m_icons;
};

#endif // NAVIGATION_MANAGER_HPP
