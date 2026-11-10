#include "torrentfilelistview.hpp"

#include "models/filestoragemodel.hpp"
#include "translator.hpp"
#include "widgets/progressrenderer.hpp"

using pt::UI::TorrentFileListView;
using pt::UI::Models::FileStorageModel;

TorrentFileListView::TorrentFileListView(wxWindow* parent, wxWindowID id)
    : wxDataViewCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE | wxBORDER_NONE, wxDefaultValidator, "TorrentFileListView")
{
    auto nameCol = AppendIconTextColumn(
        i18n("name"),
        FileStorageModel::Columns::Name,
        wxDATAVIEW_CELL_INERT,
        FromDIP(360),
        wxALIGN_LEFT);

    AppendTextColumn(
        i18n("size"),
        FileStorageModel::Columns::Size,
        wxDATAVIEW_CELL_INERT,
        FromDIP(80),
        wxALIGN_RIGHT);

    AppendColumn(
        new wxDataViewColumn(
            i18n("progress"),
            new Widgets::ProgressRenderer(),
            FileStorageModel::Columns::Progress,
            FromDIP(110),
            wxALIGN_CENTER,
            wxDATAVIEW_COL_RESIZABLE));

    AppendTextColumn(
        i18n("priority"),
        FileStorageModel::Columns::Priority,
        wxDATAVIEW_CELL_INERT,
        FromDIP(120));

    // Ugly hack to prevent the last "real" column from stretching.
    AppendColumn(new wxDataViewColumn(wxEmptyString, new wxDataViewTextRenderer(), FileStorageModel::Columns::_Max, 0));

    nameCol->GetRenderer()->EnableEllipsize(wxELLIPSIZE_END);
}

TorrentFileListView::~TorrentFileListView()
{
}
