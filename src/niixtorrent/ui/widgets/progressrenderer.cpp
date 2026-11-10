#include "progressrenderer.hpp"

#include <wx/dcclient.h>
#include <wx/graphics.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "../theme.hpp"

using pt::UI::Theme;
using pt::UI::Widgets::ProgressRenderer;

static const long KindMultiplier = 10000;

long ProgressRenderer::Encode(float progress, Kind kind)
{
    progress = std::clamp(progress, 0.0f, 1.0f);

    // Tenths of a percent, 0..1000.
    long permille = std::lround(progress * 1000.0f);

    // Never show a rounded-up "100.0 %" for something that is not finished.
    if (permille == 1000 && progress < 1.0f) { permille = 999; }

    return static_cast<long>(kind) * KindMultiplier + permille;
}

ProgressRenderer::ProgressRenderer()
    : wxDataViewCustomRenderer("long", wxDATAVIEW_CELL_INERT, wxALIGN_CENTER),
    m_value(0)
{
}

bool ProgressRenderer::SetValue(const wxVariant& value)
{
    m_value = value.IsNull() ? 0 : value.GetLong();
    return true;
}

bool ProgressRenderer::GetValue(wxVariant& value) const
{
    value = m_value;
    return true;
}

wxSize ProgressRenderer::GetSize() const
{
    wxWindow* view = GetView();
    return view != nullptr ? view->FromDIP(wxSize(100, 18)) : wxSize(100, 18);
}

#if wxUSE_ACCESSIBILITY
wxString ProgressRenderer::GetAccessibleDescription() const
{
    return wxString::Format("%.1f %%", (m_value % KindMultiplier) / 10.0);
}
#endif

bool ProgressRenderer::Render(wxRect cell, wxDC* dc, int state)
{
    wxWindow* view = GetView();
    auto dip = [view](int v) { return view != nullptr ? view->FromDIP(v) : v; };

    Kind kind = static_cast<Kind>(m_value / KindMultiplier);
    long permille = std::clamp(m_value % KindMultiplier, 0L, 1000L);

    wxColour fill;
    switch (kind)
    {
    case Kind::Complete: fill = Theme::Info();      break;
    case Kind::Checking: fill = Theme::Highlight(); break;
    case Kind::Paused:   fill = Theme::Paused();    break;
    case Kind::Error:    fill = Theme::Error();     break;
    default:             fill = Theme::Accent();    break;
    }

    bool selected = (state & wxDATAVIEW_CELL_SELECTED) != 0;
    wxColour track = selected
        ? Theme::Blend(Theme::Text(), Theme::Selection(), 0.14)
        : Theme::Track();

    // A slim bar, vertically centred in the cell.
    wxRect bar = cell;
    bar.Deflate(dip(2), 0);
    int height = std::min(cell.height - dip(4), dip(16));
    bar.y += (cell.height - height) / 2;
    bar.height = height;

    if (bar.width <= 0 || bar.height <= 0) { return true; }

    double radius = dip(3);
    int filled = static_cast<int>(std::lround(bar.width * (permille / 1000.0)));

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(*dc));

    if (gc)
    {
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetPen(*wxTRANSPARENT_PEN);

        gc->SetBrush(wxBrush(track));
        gc->DrawRoundedRectangle(bar.x, bar.y, bar.width, bar.height, radius);

        if (filled > 0)
        {
            // Clip a full-width rounded bar so the fill keeps the same corners
            // as the track at every percentage.
            gc->PushState();
            gc->Clip(bar.x, bar.y, filled, bar.height);
            gc->SetBrush(wxBrush(fill));
            gc->DrawRoundedRectangle(bar.x, bar.y, bar.width, bar.height, radius);
            gc->PopState();
        }

        gc.reset();
    }
    else
    {
        dc->SetPen(*wxTRANSPARENT_PEN);
        dc->SetBrush(wxBrush(track));
        dc->DrawRoundedRectangle(bar, radius);

        if (filled > 0)
        {
            wxDCClipper clip(*dc, wxRect(bar.x, bar.y, filled, bar.height));
            dc->SetBrush(wxBrush(fill));
            dc->DrawRoundedRectangle(bar, radius);
        }
    }

    // Percentage: one decimal while in progress, none at the ends.
    wxString label = (permille == 0 || permille == 1000)
        ? wxString::Format("%ld%%", permille / 10)
        : wxString::Format("%ld.%ld%%", permille / 10, permille % 10);

    wxFont font = dc->GetFont();
    if (!font.IsOk() && view != nullptr) { font = view->GetFont(); }
    font.SetFractionalPointSize(font.GetFractionalPointSize() * 0.9);
    wxDCFontChanger fontChanger(*dc, font);

    wxSize extent = dc->GetTextExtent(label);

    if (extent.x + dip(6) <= bar.width && extent.y <= bar.height + dip(2))
    {
        int tx = bar.x + (bar.width - extent.x) / 2;
        int ty = bar.y + (bar.height - extent.y) / 2;

        // Draw the text twice with complementary clipping so it stays
        // readable both over the filled and the empty part of the bar.
        wxColour onTrack = Theme::Text();
        wxColour onFill = Theme::TextOn(fill);

        if (filled < bar.width)
        {
            wxDCClipper clip(*dc, wxRect(bar.x + filled, bar.y, bar.width - filled, bar.height));
            wxDCTextColourChanger colour(*dc, onTrack);
            dc->DrawText(label, tx, ty);
        }

        if (filled > 0)
        {
            wxDCClipper clip(*dc, wxRect(bar.x, bar.y, filled, bar.height));
            wxDCTextColourChanger colour(*dc, onFill);
            dc->DrawText(label, tx, ty);
        }
    }

    return true;
}
