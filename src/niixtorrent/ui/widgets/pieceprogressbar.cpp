#include "pieceprogressbar.hpp"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <algorithm>
#include <memory>

#include "../theme.hpp"

namespace lt = libtorrent;
using pt::UI::Theme;
using pt::UI::Widgets::PieceProgressBar;

PieceProgressBar::PieceProgressBar(wxWindow* parent, wxWindowID id, lt::typed_bitfield<lt::piece_index_t> field)
    : wxPanel(parent, id, wxDefaultPosition, wxSize(-1, parent->FromDIP(14)), wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE),
    m_bitfield(field)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(14)));

    Bind(wxEVT_PAINT, &PieceProgressBar::OnPaint, this);
}

void PieceProgressBar::UpdateBitfield(lt::typed_bitfield<lt::piece_index_t> const& field)
{
    // The status of every torrent is polled about once a second; only repaint
    // when something actually changed.
    bool changed = field.size() != m_bitfield.size() || field.count() != m_bitfield.count();

    m_bitfield = field;

    if (changed)
    {
        Refresh(false);
    }
}

void PieceProgressBar::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);

    const wxSize size = GetClientSize();
    const wxColour background = GetParent()->GetBackgroundColour().IsOk()
        ? GetParent()->GetBackgroundColour()
        : Theme::Surface();

    dc.SetBackground(wxBrush(background));
    dc.Clear();

    if (size.x <= 2 || size.y <= 2) { return; }

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) { return; }

    const double radius = FromDIP(3);
    const wxColour track = Theme::Track();
    const wxColour fill = Theme::Accent();

    wxGraphicsPath outline = gc->CreatePath();
    outline.AddRoundedRectangle(0, 0, size.x, size.y, radius);

    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(track));
    gc->FillPath(outline);

    const int pieces = m_bitfield.size();

    if (pieces > 0)
    {
        gc->PushState();
        gc->Clip(wxRegion(wxRect(size)));
        gc->SetAntialiasMode(wxANTIALIAS_NONE);

        // Walk the pixel columns and merge neighbours of equal shade into one
        // rectangle.
        int runStart = 0;
        int runShade = -1;

        auto flush = [&](int end)
        {
            if (runShade > 0)
            {
                gc->SetBrush(wxBrush(Theme::Blend(fill, track, runShade / 8.0)));
                gc->DrawRectangle(runStart, 0, end - runStart, size.y);
            }
        };

        for (int x = 0; x < size.x; x++)
        {
            int first = static_cast<int>(static_cast<long long>(x) * pieces / size.x);
            int last = static_cast<int>(static_cast<long long>(x + 1) * pieces / size.x);
            last = std::clamp(last, first + 1, pieces);

            int have = 0;
            for (int idx = first; idx < last; idx++)
            {
                if (m_bitfield[lt::piece_index_t{ idx }]) { have++; }
            }

            // Quantised to 8 levels; any downloaded piece is at least visible.
            int shade = have == 0 ? 0 : std::max(1, have * 8 / (last - first));

            if (shade != runShade)
            {
                flush(x);
                runStart = x;
                runShade = shade;
            }
        }

        flush(size.x);
        gc->PopState();

        // Round the corners off again by painting the background outside the outline.
        wxGraphicsPath corners = gc->CreatePath();
        corners.AddRectangle(0, 0, size.x, size.y);
        corners.AddRoundedRectangle(0, 0, size.x, size.y, radius);

        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetBrush(wxBrush(background));
        gc->FillPath(corners, wxODDEVEN_RULE);
    }

    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->SetPen(wxPen(Theme::Border()));
    gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1, size.y - 1, radius);
}
