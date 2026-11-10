#include "statusbar.hpp"

#include <fmt/format.h>
#include <fmt/xchar.h>

#include <wx/dcbuffer.h>

#include "theme.hpp"
#include "translator.hpp"
#include "../core/utils.hpp"

using pt::UI::StatusBar;
using pt::UI::Theme;

StatusBar::StatusBar(wxWindow* parent)
    : wxStatusBarGeneric(parent, wxID_ANY, wxSTB_SHOW_TIPS | wxSTB_ELLIPSIZE_END | wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(Theme::Background());
    SetForegroundColour(Theme::TextMuted());
    SetMinHeight(GetCharHeight() + FromDIP(12));
    Bind(wxEVT_PAINT, &StatusBar::OnPaint, this);

    // Torrent count takes the free space; the remaining fields keep a steady
    // width so the text does not jump around while speeds change.
    int widths[] =
    {
        -1,
        FromDIP(190),
        FromDIP(300),
        FromDIP(160)
    };

    int styles[] =
    {
        wxSB_FLAT,
        wxSB_FLAT,
        wxSB_FLAT,
        wxSB_FLAT
    };

    SetFieldsCount(4);
    SetStatusWidths(4, widths);
    SetStatusStyles(4, styles);

    UpdateTransferRates(0, 0);
}

void StatusBar::UpdateDhtNodesCount(int64_t nodes)
{
    if (nodes < 0)
    {
        SetStatusText(i18n("dht_disabled"), 1);
    }
    else
    {
        SetStatusText(fmt::format(fmt::runtime(i18n("dht_num_nodes")), nodes), 1);
    }
}

void StatusBar::UpdateTorrentCount(int64_t torrents)
{
    SetStatusText(fmt::format(fmt::runtime(i18n("num_torrents")), torrents), 0);
}

void StatusBar::UpdateTransferRates(int64_t downSpeed, int64_t upSpeed)
{
    SetStatusText(fmt::format(fmt::runtime(i18n("dl_s_ul_s")), Utils::toHumanFileSize(downSpeed), Utils::toHumanFileSize(upSpeed)), 2);
}

void StatusBar::UpdateIPFilterStatus(bool enabled)
{
    if (enabled)
    {
        SetStatusText(i18n("ip_filter_enabled"), 3);
    }
    else
    {
        SetStatusText(i18n("ip_filter_disabled"), 3);
    }
}

void StatusBar::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize client = GetClientSize();

    dc.SetBackground(wxBrush(Theme::Background()));
    dc.Clear();

    // Thin pink edge along the top, separators as plain hairlines.
    dc.SetPen(wxPen(Theme::AccentEdge()));
    dc.DrawLine(0, 0, client.x, 0);
    dc.SetPen(wxPen(Theme::Border()));

    wxFont mono = Theme::MonoFont(this);
    mono.SetFractionalPointSize(GetFont().GetFractionalPointSize() * 0.92);
    dc.SetFont(mono);
    dc.SetTextForeground(Theme::TextMuted());

    int pad = FromDIP(10);

    for (int i = 0; i < GetFieldsCount(); i++)
    {
        wxRect rect;
        if (!GetFieldRect(i, rect)) { continue; }

        rect.y = 1;
        rect.height = client.y - 1;

        // Separator before every field but the first.
        if (i > 0)
        {
            int inset = FromDIP(6);
            dc.DrawLine(rect.x, rect.y + inset, rect.x, rect.GetBottom() - inset + 1);
        }

        wxRect text = rect;
        text.Deflate(pad, 0);

        wxString label = wxControl::Ellipsize(GetStatusText(i), dc, wxELLIPSIZE_END, text.width);
        dc.DrawLabel(label, text, (i == 0 ? wxALIGN_LEFT : wxALIGN_LEFT) | wxALIGN_CENTER_VERTICAL);
    }
}
