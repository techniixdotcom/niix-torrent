#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/taskbar.h>

namespace pt
{
namespace UI
{
    // The notification area (system tray) icon and its menu.
    class TaskBarIcon : public wxTaskBarIcon
    {
    public:
        TaskBarIcon(wxWindow* parent);

        void Hide();
        void Show();

        // Shows a desktop notification: a balloon from the tray icon on
        // Windows, a regular notification elsewhere.
        void Notify(wxString const& title, wxString const& text);

    private:
        wxMenu* CreatePopupMenu() wxOVERRIDE;

        wxWindow* m_parent;
    };
}
}
