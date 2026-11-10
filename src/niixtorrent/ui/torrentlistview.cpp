#include "torrentlistview.hpp"

#include <wx/persist.h>

#include "../bittorrent/torrenthandle.hpp"
#include "models/torrentlistmodel.hpp"
#include "persistence/persistenttorrentlistview.hpp"
#include "translator.hpp"
#include "widgets/progressrenderer.hpp"

#include <algorithm>

using pt::UI::TorrentListView;
using pt::UI::Models::TorrentListModel;

TorrentListView::TorrentListView(wxWindow* parent, wxWindowID id, pt::UI::Models::TorrentListModel* model)
    : wxDataViewCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE | wxBORDER_NONE, wxDefaultValidator, "TorrentListView"),
    m_model(model)
{
    this->AssociateModel(m_model);
    m_model->DecRef();

    // A little more air than the default row height (generic control only;
    // GTK sizes rows from the theme).
    this->SetRowHeight(std::max(GetCharHeight() + FromDIP(10), FromDIP(26)));

    auto nameRenderer = new wxDataViewTextRenderer();
    nameRenderer->EnableEllipsize(wxELLIPSIZE_END);

    auto statusRenderer = new wxDataViewTextRenderer();
    statusRenderer->EnableEllipsize(wxELLIPSIZE_END);

    auto defaultFlags = wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE;

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("name"),
                nameRenderer,
                TorrentListModel::Columns::Name,
                FromDIP(320),
                wxALIGN_NOT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("queue_position"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::QueuePosition,
                FromDIP(40),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("size"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::Size,
                FromDIP(80),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("size_remaining"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::SizeRemaining,
                FromDIP(80),
                wxALIGN_RIGHT,
                defaultFlags),
            true));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("status"),
                statusRenderer,
                TorrentListModel::Columns::Status,
                FromDIP(130),
                wxALIGN_NOT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("progress"),
                new Widgets::ProgressRenderer(),
                TorrentListModel::Columns::Progress,
                FromDIP(140),
                wxALIGN_CENTER,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("eta"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::ETA,
                FromDIP(80),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("dl"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::DownloadSpeed,
                FromDIP(80),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("ul"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::UploadSpeed,
                FromDIP(80),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("availability"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::Availability,
                FromDIP(80),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("ratio"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::Ratio,
                FromDIP(80),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("seeds"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::Seeds,
                FromDIP(80),
                wxALIGN_RIGHT,
                wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("peers"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::Peers,
                FromDIP(80),
                wxALIGN_RIGHT,
                wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("added_on"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::AddedOn,
                FromDIP(140),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("completed_on"),
                new wxDataViewTextRenderer(),
                TorrentListModel::Columns::CompletedOn,
                FromDIP(140),
                wxALIGN_RIGHT,
                defaultFlags)));

    m_columns.push_back(
        ColumnMetadata(
            new wxDataViewColumn(
                i18n("label"),
                new wxDataViewIconTextRenderer(),
                TorrentListModel::Columns::Label,
                FromDIP(80),
                wxALIGN_LEFT,
                defaultFlags),
            true));

    // Saved column order, widths and visibility are applied while the columns
    // are inserted. (Reordering afterwards would mean deleting and re-adding
    // columns, and since wxWidgets 3.3 deleting a column destroys it.)
    auto persistent = static_cast<Persistence::PersistentTorrentListView*>(
        wxPersistenceManager::Get().Register(this));

    Persistence::PersistentTorrentListView::Layout layout;
    std::vector<ColumnMetadata> ordered;

    if (persistent != nullptr
        && persistent->LoadLayout(layout)
        && layout.models.size() == m_columns.size())
    {
        for (auto model : layout.models)
        {
            auto it = std::find_if(
                m_columns.begin(),
                m_columns.end(),
                [model](ColumnMetadata const& cmd) { return static_cast<int>(cmd.column->GetModelColumn()) == model; });

            if (it == m_columns.end()) { break; }
            ordered.push_back(*it);
        }
    }

    if (ordered.size() == m_columns.size())
    {
        for (size_t i = 0; i < ordered.size(); i++)
        {
            InsertColumn(GetColumnCount(), ordered[i].column);
            ordered[i].column->SetHidden(layout.hidden[i]);
            if (layout.widths[i] > 0) { ordered[i].column->SetWidth(layout.widths[i]); }
        }

        if (layout.sortIndex >= 0 && layout.sortIndex < static_cast<int>(GetColumnCount()))
        {
            GetColumn(layout.sortIndex)->SetSortOrder(layout.sortAscending);
        }
    }
    else
    {
        for (auto const& cmd : m_columns)
        {
            InsertColumn(GetColumnCount(), cmd.column);
            cmd.column->SetHidden(cmd.hidden);
        }
    }

    // insert the "fake" column last, always
    AppendColumn(new wxDataViewColumn(wxEmptyString, new wxDataViewTextRenderer(), TorrentListModel::Columns::_Max, 0, wxALIGN_CENTER, 0));

    // Keyboard accelerators
    std::vector<wxAcceleratorEntry> entries =
    {
        wxAcceleratorEntry(wxACCEL_CTRL,   int('A'),   ptID_KEY_SELECT_ALL),
        wxAcceleratorEntry(wxACCEL_NORMAL, WXK_DELETE, ptID_KEY_DELETE),
        wxAcceleratorEntry(wxACCEL_SHIFT,  WXK_DELETE, ptID_KEY_DELETE_FILES),
    };

    this->SetAcceleratorTable(wxAcceleratorTable(static_cast<int>(entries.size()), entries.data()));

    this->Bind(wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, &TorrentListView::ShowHeaderContextMenu, this);

    this->Bind(
        wxEVT_MENU,
        [&](wxCommandEvent&)
        {
            wxDataViewItemArray items;
            this->GetSelections(items);

            if (items.IsEmpty()) { return; }

            for (wxDataViewItem& item : items)
            {
                m_model->GetTorrentFromItem(item)->Remove();
            }
        },
        ptID_KEY_DELETE);

    this->Bind(
        wxEVT_MENU,
        [&](wxCommandEvent&)
        {
            wxDataViewItemArray items;
            this->GetSelections(items);

            if (items.IsEmpty()) { return; }

            if (wxMessageBox(
                i18n("confirm_remove_description"),
                i18n("confirm_remove"),
                wxOK | wxCANCEL | wxICON_INFORMATION,
                m_parent) != wxOK) {
                return;
            }

            for (wxDataViewItem& item : items)
            {
                m_model->GetTorrentFromItem(item)->RemoveFiles();
            }
        },
        ptID_KEY_DELETE_FILES);

    this->Bind(
        wxEVT_MENU,
        [&](wxCommandEvent&)
        {
            this->SelectAll();
            wxPostEvent(
                GetParent(),
                wxCommandEvent(wxEVT_DATAVIEW_SELECTION_CHANGED, this->GetId()));
        },
        ptID_KEY_SELECT_ALL);
}

TorrentListView::~TorrentListView()
{
}

wxDataViewColumn* TorrentListView::ColumnAtPosition(unsigned int position) const
{
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    return GetColumnAt(position);
#else
    for (unsigned int i = 0; i < GetColumnCount(); i++)
    {
        wxDataViewColumn* column = GetColumn(i);
        if (GetColumnPosition(column) == static_cast<int>(position)) { return column; }
    }

    return GetColumn(position);
#endif
}

int TorrentListView::IndexOfColumn(const wxDataViewColumn* column) const
{
    for (unsigned int i = 0; i < GetColumnCount(); i++)
    {
        if (GetColumn(i) == column) { return static_cast<int>(i); }
    }

    return wxNOT_FOUND;
}

void TorrentListView::ShowHeaderContextMenu(wxCommandEvent&)
{
    wxMenu menu;

    // Do not iterate through the last column since it is the one we
    // append to prevent the last real column to stretch

    for (size_t i = 0; i < GetColumnCount() - 1; i++)
    {
        auto col = ColumnAtPosition(i);
        auto item = menu.Append(wxID_HIGHEST + i, col->GetTitle());
        item->SetCheckable(true);
        item->Check(!col->IsHidden());
    }

    menu.Bind(
        wxEVT_MENU,
        [this](wxCommandEvent& evt)
        {
            auto col = ColumnAtPosition(evt.GetId() - wxID_HIGHEST);
            col->SetHidden(!col->IsHidden());
        });

    PopupMenu(&menu);
}
