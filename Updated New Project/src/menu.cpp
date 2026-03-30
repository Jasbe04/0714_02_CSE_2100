/**
 * @file menu.cpp
 * @brief SRP: clipboard operations — nothing else.
 *        OCP + LSP: uses FileOperation polymorphism for paste.
 */

#include "menu.hpp"
#include "file_op.hpp"

#include <unistd.h>
#include <string>

ClipboardManager::ClipboardManager(AppState          *state,
                                   IProgressReporter *reporter,
                                   INavigable        *nav)
    : m_state(state), m_reporter(reporter), m_nav(nav)
{}

void ClipboardManager::markCopy(const char *dir, const char *name)
{
    m_state->clipboardDir  = dir;
    m_state->clipboardName = name;
    m_state->isCopy        = true;
    m_state->isCut         = false;
    gtk_widget_set_opacity(m_state->event4, 1.0);
}

void ClipboardManager::markCut(const char *dir, const char *name)
{
    m_state->clipboardDir  = dir;
    m_state->clipboardName = name;
    m_state->isCut         = true;
    m_state->isCopy        = false;
    gtk_widget_set_opacity(m_state->event4, 1.0);
}

void ClipboardManager::paste()
{
    if (!m_state->isCopy && !m_state->isCut)
        return;

    char destFolder[MAX_PATH + 1];
    getcwd(destFolder, sizeof(destFolder));

    std::string src  = m_state->clipboardDir + "\\" + m_state->clipboardName;
    std::string dest = std::string(destFolder) + "\\" + m_state->clipboardName;

    // OCP + LSP: choose the right FileOperation subclass; no if-else needed
    //            anywhere else.
    if (m_state->isCopy)
    {
        CopyOperation op(src, dest);
        op.execute(m_reporter);
    }
    else // isCut
    {
        MoveOperation op(src, dest);
        op.execute(m_reporter);
    }

    m_state->isCut  = false;
    m_state->isCopy = false;
    gtk_widget_set_opacity(m_state->event4, 0.1);

    m_nav->openDirectory();
}

bool ClipboardManager::hasPending() const
{
    return m_state->isCopy || m_state->isCut;
}
