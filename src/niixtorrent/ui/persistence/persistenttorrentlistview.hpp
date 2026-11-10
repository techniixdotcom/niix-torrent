#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/persist/window.h>

#include <vector>

#include "../torrentlistview.hpp"

namespace pt
{
namespace UI
{
namespace Persistence
{
    class PersistentTorrentListView : public wxPersistentWindow<TorrentListView>
    {
    public:
        // The saved column layout, in display order.
        struct Layout
        {
            std::vector<int>  models;   // model column of each position
            std::vector<bool> hidden;
            std::vector<int>  widths;
            int  sortIndex = -1;
            bool sortAscending = false;
        };

        PersistentTorrentListView(TorrentListView* lv);
        virtual wxString GetKind() const wxOVERRIDE;

        // Reads the saved layout; TorrentListView applies it while it adds
        // its columns.
        bool LoadLayout(Layout& layout);

    protected:
        bool Restore() wxOVERRIDE;
        void Save() const wxOVERRIDE;

    private:
        TorrentListView* m_tlv;
    };
}
}
}

pt::UI::Persistence::PersistentTorrentListView* wxCreatePersistentObject(pt::UI::TorrentListView* pdvc);

