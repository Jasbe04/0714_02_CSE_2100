/**
 * @file search.hpp
 * @brief SRP — Single Responsibility: file search only.
 *        ISP — Implements ISearchable; other modules never see this class.
 *
 * Wraps the search logic from search.c.
 *
 * SOLID principles demonstrated here:
 *   S (SRP) — Searches for files.  Does not navigate or manage clipboard.
 *   I (ISP) — Implements ISearchable; UIController depends on this interface.
 */

#ifndef SEARCH_SERVICE_HPP
#define SEARCH_SERVICE_HPP

#include "interfaces.hpp"
#include "app_state.hpp"
#include "icon.hpp"

/**
 * @brief Implements recursive case-insensitive file search.
 */
class SearchService : public ISearchable
{
public:
    /**
     * @brief Constructs the service.
     * @param state Shared state (store + icon pixbuf).
     * @param icons IconLoader for per-file icons in the results list.
     */
    SearchService(AppState *state, IconLoader *icons);

    // ── ISearchable ───────────────────────────────────────────────────────────

    /**
     * @brief Clears the list store and populates it with matching entries.
     * @param query    Substring to match (case-insensitive).
     * @param basePath Directory root for the recursive search.
     */
    void search(const char *query, const char *basePath) override;

private:
    /**
     * @brief Recursive search helper.
     * @param dir   Current directory being scanned.
     * @param query Lowercased query string.
     */
    void searchRecursive(const char *dir, const char *query);

    AppState   *m_state;
    IconLoader *m_icons;
};

#endif // SEARCH_SERVICE_HPP
