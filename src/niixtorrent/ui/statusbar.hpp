#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/statusbr.h>

#include <memory>

namespace pt
{
namespace UI
{
    // Owner-drawn on every platform (the native Windows status bar ignores
    // the dark theme): muted text in fixed fields with hairline separators.
    class StatusBar : public wxStatusBar
    {
    public:
        StatusBar(wxWindow* parent);

        void UpdateDhtNodesCount(int64_t nodes);
        void UpdateIPFilterStatus(bool enabled);
        void UpdateTorrentCount(int64_t torrents);
        void UpdateTransferRates(int64_t downSpeed, int64_t upSpeed);

    private:
        void OnPaint(wxPaintEvent&);
    };
}
}
