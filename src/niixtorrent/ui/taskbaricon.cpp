#include "taskbaricon.hpp"

#include <wx/notifmsg.h>

#include "icons.hpp"
#include "ids.hpp"
#include "translator.hpp"

using pt::UI::TaskBarIcon;

TaskBarIcon::TaskBarIcon(wxWindow* parent)
    : m_parent(parent)
{
}

void TaskBarIcon::Hide()
{
    if (IsIconInstalled())
    {
        RemoveIcon();
    }
}

void TaskBarIcon::Show()
{
    // The bundle lets the platform pick the size matching the tray's DPI.
    SetIcon(wxBitmapBundle::FromIconBundle(Icons::App()), "NiiX Torrent");
}

void TaskBarIcon::Notify(wxString const& title, wxString const& text)
{
#ifdef __WXMSW__
    if (IsIconInstalled())
    {
        ShowBalloon(title, text);
        return;
    }
#endif

    wxNotificationMessage notification(title, text, m_parent);
    notification.SetIcon(Icons::App().GetIcon(wxSize(48, 48), wxIconBundle::FALLBACK_NEAREST_LARGER));
    notification.Show();
}

wxMenu* TaskBarIcon::CreatePopupMenu()
{
    wxMenu* menu = new wxMenu();
    // Some Linux trays open this menu on every click, so the window must be
    // reachable from here.
    menu->Append(ptID_EVT_SHOW_WINDOW, i18n("show_niixtorrent"));
    menu->AppendSeparator();
    menu->Append(ptID_EVT_ADD_TORRENT, i18n("amp_add_torrent"));
    menu->Append(ptID_EVT_ADD_MAGNET_LINK, i18n("amp_add_magnet_link_s"));
    menu->AppendSeparator();
    menu->Append(ptID_EVT_VIEW_PREFERENCES, i18n("preferences"));
    menu->AppendSeparator();
    menu->Append(ptID_EVT_EXIT, i18n("amp_exit"));

    return menu;
}
