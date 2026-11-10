#include "tabbook.hpp"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <algorithm>
#include <memory>

#include "../theme.hpp"

using pt::UI::Theme;
using pt::UI::Widgets::TabBar;
using pt::UI::Widgets::TabBook;

TabBar::TabBar(wxWindow* parent, wxWindowID id, wxOrientation orientation)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE),
    m_orientation(orientation),
    m_selection(wxNOT_FOUND),
    m_hover(wxNOT_FOUND)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    Bind(wxEVT_PAINT, &TabBar::OnPaint, this);
    Bind(wxEVT_SIZE, &TabBar::OnSize, this);
    Bind(wxEVT_MOTION, &TabBar::OnMouseMove, this);
    Bind(wxEVT_LEAVE_WINDOW, &TabBar::OnMouseLeave, this);
    Bind(wxEVT_LEFT_DOWN, &TabBar::OnLeftDown, this);
    Bind(wxEVT_KEY_DOWN, &TabBar::OnKeyDown, this);
    Bind(wxEVT_SET_FOCUS, &TabBar::OnFocus, this);
    Bind(wxEVT_KILL_FOCUS, &TabBar::OnFocus, this);
}

int TabBar::AddTab(wxString const& label)
{
    m_tabs.push_back({ label, wxRect() });

    if (m_selection == wxNOT_FOUND) { m_selection = 0; }

    InvalidateBestSize();
    LayoutTabs();
    Refresh();

    return static_cast<int>(m_tabs.size()) - 1;
}

void TabBar::SetSelection(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) { return; }

    m_selection = index;
    Refresh();
}

void TabBar::Select(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size()) || index == m_selection) { return; }

    m_selection = index;
    Refresh();

    wxCommandEvent evt(wxEVT_CHOICE, GetId());
    evt.SetEventObject(this);
    evt.SetInt(index);
    ProcessWindowEvent(evt);
}

wxSize TabBar::DoGetBestClientSize() const
{
    wxClientDC dc(const_cast<TabBar*>(this));
    dc.SetFont(GetFont());

    int padX = FromDIP(14);

    if (m_orientation == wxHORIZONTAL)
    {
        int width = FromDIP(6);
        for (auto const& tab : m_tabs) { width += dc.GetTextExtent(tab.label).x + 2 * padX; }
        return wxSize(width, GetCharHeight() + FromDIP(22));
    }

    int width = FromDIP(140);
    for (auto const& tab : m_tabs) { width = std::max(width, dc.GetTextExtent(tab.label).x + 2 * padX + FromDIP(8)); }

    int rowHeight = GetCharHeight() + FromDIP(16);
    return wxSize(width, static_cast<int>(m_tabs.size()) * (rowHeight + FromDIP(2)) + FromDIP(8));
}

void TabBar::LayoutTabs()
{
    wxClientDC dc(this);
    dc.SetFont(GetFont());

    wxSize client = GetClientSize();
    int padX = FromDIP(14);

    if (m_orientation == wxHORIZONTAL)
    {
        int x = FromDIP(8);
        for (auto& tab : m_tabs)
        {
            int w = dc.GetTextExtent(tab.label).x + 2 * padX;
            tab.rect = wxRect(x, 0, w, client.y);
            x += w + FromDIP(2);
        }
        return;
    }

    int rowHeight = GetCharHeight() + FromDIP(16);
    int y = FromDIP(4);

    for (auto& tab : m_tabs)
    {
        tab.rect = wxRect(FromDIP(4), y, client.x - FromDIP(8), rowHeight);
        y += rowHeight + FromDIP(2);
    }
}

int TabBar::HitTest(wxPoint const& pt) const
{
    for (size_t i = 0; i < m_tabs.size(); i++)
    {
        if (m_tabs[i].rect.Contains(pt)) { return static_cast<int>(i); }
    }

    return wxNOT_FOUND;
}

void TabBar::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize client = GetClientSize();

    bool horizontal = m_orientation == wxHORIZONTAL;
    wxColour background = horizontal ? Theme::Background() : Theme::Surface();

    dc.SetBackground(wxBrush(background));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) { return; }

    gc->SetFont(GetFont(), Theme::Text());
    gc->SetPen(*wxTRANSPARENT_PEN);

    if (horizontal)
    {
        // Thin pink edge under the strip, as on the niixaOS header.
        gc->SetBrush(wxBrush(Theme::AccentEdge()));
        gc->DrawRectangle(0, client.y - FromDIP(1), client.x, FromDIP(1));
    }

    double radius = FromDIP(8);

    for (size_t i = 0; i < m_tabs.size(); i++)
    {
        Tab const& tab = m_tabs[i];
        bool selected = static_cast<int>(i) == m_selection;
        bool hovered = static_cast<int>(i) == m_hover;

        // Pills, inset from the tab's cell.
        wxRect r = tab.rect;
        if (horizontal) { r.Deflate(FromDIP(2), FromDIP(6)); r.y -= FromDIP(1); }

        if (selected)
        {
            // Pink wash with a 1 px pink edge.
            gc->SetBrush(wxBrush(Theme::AccentTint()));
            gc->SetPen(wxPen(Theme::Blend(Theme::Accent(), background, 0.6), FromDIP(1)));
            gc->DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, radius);
            gc->SetPen(*wxTRANSPARENT_PEN);
        }
        else if (hovered)
        {
            gc->SetBrush(wxBrush(Theme::Blend(Theme::Accent(), background, Theme::IsDark() ? 0.10 : 0.06)));
            gc->DrawRoundedRectangle(r.x, r.y, r.width, r.height, radius);
        }

        wxFont font = GetFont();
        if (selected) { font.SetWeight(wxFONTWEIGHT_SEMIBOLD); }
        gc->SetFont(font, selected || hovered ? Theme::Text() : Theme::TextMuted());

        double tw = 0, th = 0;
        gc->GetTextExtent(tab.label, &tw, &th);

        double tx = horizontal ? r.x + (r.width - tw) / 2.0 : r.x + FromDIP(14);
        double ty = r.y + (r.height - th) / 2.0;
        gc->DrawText(tab.label, tx, ty);

        if (selected && HasFocus())
        {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(Theme::Accent(), FromDIP(1), wxPENSTYLE_DOT));
            gc->DrawRoundedRectangle(r.x + FromDIP(3), r.y + FromDIP(3), r.width - FromDIP(6), r.height - FromDIP(6), radius - FromDIP(3));
            gc->SetPen(*wxTRANSPARENT_PEN);
        }
    }
}

void TabBar::OnSize(wxSizeEvent& evt)
{
    LayoutTabs();
    Refresh();
    evt.Skip();
}

void TabBar::OnMouseMove(wxMouseEvent& evt)
{
    int hover = HitTest(evt.GetPosition());

    if (hover != m_hover)
    {
        m_hover = hover;
        SetCursor(hover == wxNOT_FOUND ? wxNullCursor : wxCursor(wxCURSOR_HAND));
        Refresh();
    }
}

void TabBar::OnMouseLeave(wxMouseEvent&)
{
    if (m_hover != wxNOT_FOUND)
    {
        m_hover = wxNOT_FOUND;
        Refresh();
    }
}

void TabBar::OnLeftDown(wxMouseEvent& evt)
{
    int index = HitTest(evt.GetPosition());
    if (index != wxNOT_FOUND) { Select(index); }
}

void TabBar::OnKeyDown(wxKeyEvent& evt)
{
    bool horizontal = m_orientation == wxHORIZONTAL;

    switch (evt.GetKeyCode())
    {
    case WXK_LEFT:  if (horizontal)  { Select(m_selection - 1); return; } break;
    case WXK_RIGHT: if (horizontal)  { Select(m_selection + 1); return; } break;
    case WXK_UP:    if (!horizontal) { Select(m_selection - 1); return; } break;
    case WXK_DOWN:  if (!horizontal) { Select(m_selection + 1); return; } break;
    case WXK_HOME:  Select(0); return;
    case WXK_END:   Select(static_cast<int>(m_tabs.size()) - 1); return;
    case WXK_TAB:   Navigate(evt.ShiftDown() ? wxNavigationKeyEvent::IsBackward : wxNavigationKeyEvent::IsForward); return;
    default: break;
    }

    evt.Skip();
}

void TabBar::OnFocus(wxFocusEvent& evt)
{
    Refresh();
    evt.Skip();
}

TabBook::TabBook(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE),
    m_bar(new TabBar(this, wxID_ANY, wxHORIZONTAL)),
    m_book(new wxSimplebook(this, wxID_ANY))
{
    SetBackgroundColour(Theme::Background());

    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_bar, 0, wxEXPAND);
    sizer->Add(m_book, 1, wxEXPAND);
    SetSizer(sizer);

    m_bar->Bind(wxEVT_CHOICE, [this](wxCommandEvent& evt) { m_book->ChangeSelection(evt.GetInt()); });
}

void TabBook::AddPage(wxWindow* page, wxString const& label, bool select)
{
    m_book->AddPage(page, label, false);
    int index = m_bar->AddTab(label);

    if (select || m_book->GetPageCount() == 1)
    {
        SetSelection(index);
    }

    Layout();
}

void TabBook::SetSelection(int index)
{
    m_bar->SetSelection(index);
    m_book->ChangeSelection(index);
}
