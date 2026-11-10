#include "persistenttorrentlistview.hpp"

#include <map>

#include <nlohmann/json.hpp>

#include "../models/torrentlistmodel.hpp"
#include "../torrentlistview.hpp"

using json = nlohmann::json;
using pt::UI::TorrentListView;
using pt::UI::Models::TorrentListModel;
using pt::UI::Persistence::PersistentTorrentListView;

static std::map<TorrentListModel::Columns, std::string> ColumnNames =
{
    { TorrentListModel::Columns::Name,          "Col_Name" },
    { TorrentListModel::Columns::QueuePosition, "Col_QueuePosition" },
    { TorrentListModel::Columns::Size,          "Col_Size" },
    { TorrentListModel::Columns::SizeRemaining, "Col_SizeRemaining" },
    { TorrentListModel::Columns::Status,        "Col_Status" },
    { TorrentListModel::Columns::Progress,      "Col_Progress" },
    { TorrentListModel::Columns::ETA,           "Col_ETA" },
    { TorrentListModel::Columns::DownloadSpeed, "Col_DownloadSpeed" },
    { TorrentListModel::Columns::UploadSpeed,   "Col_UploadSpeed" },
    { TorrentListModel::Columns::Availability,  "Col_Availability" },
    { TorrentListModel::Columns::Ratio,         "Col_Ratio" },
    { TorrentListModel::Columns::Seeds,         "Col_Seeds" },
    { TorrentListModel::Columns::Peers,         "Col_Peers" },
    { TorrentListModel::Columns::AddedOn,       "Col_AddedOn" },
    { TorrentListModel::Columns::CompletedOn,   "Col_CompletedOn" },
    { TorrentListModel::Columns::Label,         "Col_Label" },
};

PersistentTorrentListView::PersistentTorrentListView(TorrentListView* tlv)
    : wxPersistentWindow<TorrentListView>(tlv),
    m_tlv(tlv)
{
}

wxString PersistentTorrentListView::GetKind() const
{
    return "torrentViewCtrl";
}

bool PersistentTorrentListView::Restore()
{
    // Applied by TorrentListView itself (see LoadLayout).
    return true;
}

bool PersistentTorrentListView::LoadLayout(Layout& layout)
{
    wxString hiddenJson;
    wxString modelsJson;
    wxString widthsJson;

    if (!RestoreValue("Hidden", &hiddenJson)
        || !RestoreValue("Models", &modelsJson)
        || !RestoreValue("Widths", &widthsJson))
    {
        return false;
    }

    try
    {
        json hidden = json::parse(hiddenJson.utf8_string());
        json models = json::parse(modelsJson.utf8_string());
        json widths = json::parse(widthsJson.utf8_string());

        if (!hidden.is_array() || !models.is_array() || !widths.is_array()
            || hidden.size() != models.size() || widths.size() != models.size())
        {
            return false;
        }

        layout.models.clear();
        layout.hidden.clear();
        layout.widths.clear();

        for (size_t i = 0; i < models.size(); i++)
        {
            layout.models.push_back(models[i].get<int>());
            layout.hidden.push_back(hidden[i].get<bool>());
            layout.widths.push_back(widths[i].get<int>());
        }
    }
    catch (std::exception const&)
    {
        return false;
    }

    int sortIndex = -1;
    bool sortAscending = false;

    if (RestoreValue("SortIndex", &sortIndex) && RestoreValue("SortAscending", &sortAscending))
    {
        layout.sortIndex = sortIndex;
        layout.sortAscending = sortAscending;
    }

    return true;
}

void PersistentTorrentListView::Save() const
{
    if (auto sortingColumn = m_tlv->GetSortingColumn())
    {
        SaveValue("SortIndex", m_tlv->IndexOfColumn(sortingColumn));
        SaveValue("SortAscending", sortingColumn->IsSortOrderAscending());
    }

    json hidden;
    json models;
    json widths;

    // subtract one from the column count since we have a fake column
    // at the very end

    for (uint32_t i = 0; i < m_tlv->GetColumnCount() - 1; i++)
    {
        auto col = m_tlv->ColumnAtPosition(i);

        hidden.push_back(col->IsHidden());
        models.push_back(col->GetModelColumn());
        widths.push_back(col->GetWidth());
    }

    SaveValue("Hidden", hidden.dump());
    SaveValue("Models", models.dump());
    SaveValue("Widths", widths.dump());
}

PersistentTorrentListView* wxCreatePersistentObject(TorrentListView* lv)
{
    return new PersistentTorrentListView(lv);
}
