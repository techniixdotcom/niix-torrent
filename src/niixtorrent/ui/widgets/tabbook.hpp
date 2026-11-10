#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/simplebook.h>

#include <vector>

namespace pt::UI::Widgets
{
    // An owner-drawn tab strip. It looks the same on every platform and in
    // both themes, unlike the native notebook, whose dark mode rendering on
    // Windows does not match the rest of the window.
    //
    //   Horizontal: labels in a row, the active one underlined in the accent
    //               colour (details pane, dialogs).
    //   Vertical:   a navigation list, the active entry filled and marked
    //               with an accent bar (Preferences).
    class TabBar : public wxControl
    {
    public:
        TabBar(wxWindow* parent, wxWindowID id, wxOrientation orientation = wxHORIZONTAL);

        int AddTab(wxString const& label);
        void SetSelection(int index);   // sends no event
        int GetSelection() const { return m_selection; }
        size_t GetTabCount() const { return m_tabs.size(); }

    protected:
        wxSize DoGetBestClientSize() const override;
        bool AcceptsFocusFromKeyboard() const override { return true; }

    private:
        struct Tab
        {
            wxString label;
            wxRect rect;
        };

        void LayoutTabs();
        int HitTest(wxPoint const& pt) const;
        void Select(int index); // with event

        void OnPaint(wxPaintEvent&);
        void OnSize(wxSizeEvent&);
        void OnMouseMove(wxMouseEvent&);
        void OnMouseLeave(wxMouseEvent&);
        void OnLeftDown(wxMouseEvent&);
        void OnKeyDown(wxKeyEvent&);
        void OnFocus(wxFocusEvent&);

        wxOrientation m_orientation;
        std::vector<Tab> m_tabs;
        int m_selection;
        int m_hover;
    };

    // A TabBar on top of a wxSimplebook. Pages must be created with
    // GetPageParent() as their parent.
    class TabBook : public wxPanel
    {
    public:
        TabBook(wxWindow* parent, wxWindowID id);

        wxWindow* GetPageParent() const { return m_book; }

        void AddPage(wxWindow* page, wxString const& label, bool select = false);
        int GetSelection() const { return m_book->GetSelection(); }
        void SetSelection(int index);

    private:
        TabBar* m_bar;
        wxSimplebook* m_book;
    };
}
