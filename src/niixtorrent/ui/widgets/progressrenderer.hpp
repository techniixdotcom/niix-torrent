#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/dataview.h>

namespace pt
{
namespace UI
{
namespace Widgets
{
    // A flat, rounded progress bar with a centred percentage, coloured by the
    // state of the item it belongs to. Replaces wxDataViewProgressRenderer,
    // which draws a native (and on GTK very tall, on Windows always green) bar.
    class ProgressRenderer : public wxDataViewCustomRenderer
    {
    public:
        enum class Kind
        {
            Active,     // downloading
            Complete,   // seeding / finished
            Checking,
            Paused,
            Error
        };

        // Packs a 0..1 progress value and its kind into the long the model returns.
        static long Encode(float progress, Kind kind);

        ProgressRenderer();

        bool SetValue(const wxVariant& value) override;
        bool GetValue(wxVariant& value) const override;
        bool Render(wxRect cell, wxDC* dc, int state) override;
        wxSize GetSize() const override;

#if wxUSE_ACCESSIBILITY
        wxString GetAccessibleDescription() const override;
#endif

    private:
        long m_value;
    };
}
}
}
