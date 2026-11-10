#include "aboutdialog.hpp"

#include <sstream>

#include <boost/version.hpp>
#include <fmt/core.h>
#include <libtorrent/version.hpp>
#include <nlohmann/json.hpp>
#include <openssl/opensslv.h>
#include <sqlite3.h>
#include <wx/hyperlink.h>
#include <wx/listctrl.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/version.h>

#include "../../buildinfo.hpp"
#include "../icons.hpp"
#include "../theme.hpp"
#include "../translator.hpp"

using json = nlohmann::json;
using pt::UI::Dialogs::AboutDialog;
using pt::UI::Icons;
using pt::UI::Theme;

AboutDialog::AboutDialog(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, i18n("about_niixtorrent"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // 96 DIP logo, taken from the largest artwork so it stays sharp when scaled.
    auto bmp = new wxStaticBitmap(
        this,
        wxID_ANY,
        wxBitmapBundle::FromBitmap(Icons::AppBitmap(FromDIP(96))));

    auto title = new wxStaticText(this, wxID_ANY, "NiiX Torrent");
    wxFont titleFont = title->GetFont();
    titleFont.SetFractionalPointSize(titleFont.GetFractionalPointSize() * 1.6);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);

    auto version = new wxStaticText(
        this,
        wxID_ANY,
        wxString::Format("%s (%s)", BuildInfo::semver(), BuildInfo::commitish()));
    version->SetForegroundColour(Theme::TextMuted());

    auto lv = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    lv->AppendColumn(i18n("component"), wxLIST_FORMAT_LEFT, FromDIP(210));
    lv->AppendColumn(i18n("version"), wxLIST_FORMAT_LEFT, FromDIP(110));

    std::stringstream boostVersion;
    boostVersion << BOOST_VERSION / 100000 << "."
        << BOOST_VERSION / 100 % 1000 << "."
        << BOOST_VERSION % 100;

    std::stringstream fmtVersion;
    fmtVersion << FMT_VERSION / 10000 << "."
        << FMT_VERSION % 10000 / 100 << "."
        << FMT_VERSION % 100;

    std::stringstream nljson;
    nljson << NLOHMANN_JSON_VERSION_MAJOR << "."
        << NLOHMANN_JSON_VERSION_MINOR << "."
        << NLOHMANN_JSON_VERSION_PATCH;

    lv->InsertItem(lv->GetItemCount(), "Boost");
    lv->SetItem(lv->GetItemCount() - 1, 1, boostVersion.str());

    lv->InsertItem(lv->GetItemCount(), "fmt");
    lv->SetItem(lv->GetItemCount() - 1, 1, fmtVersion.str());

    lv->InsertItem(lv->GetItemCount(), "nlohmann-json");
    lv->SetItem(lv->GetItemCount() - 1, 1, nljson.str());

    lv->InsertItem(lv->GetItemCount(), "OpenSSL");
    lv->SetItem(lv->GetItemCount() - 1, 1, OPENSSL_FULL_VERSION_STR);

    lv->InsertItem(lv->GetItemCount(), "Rasterbar-libtorrent");
    lv->SetItem(lv->GetItemCount() - 1, 1, LIBTORRENT_VERSION);

    lv->InsertItem(lv->GetItemCount(), "SQLite");
    lv->SetItem(lv->GetItemCount() - 1, 1, SQLITE_VERSION);

    lv->InsertItem(lv->GetItemCount(), "wxWidgets");
    lv->SetItem(lv->GetItemCount() - 1, 1, wxVERSION_NUM_DOT_STRING_T);

    auto desc = new wxStaticText(this, wxID_ANY, i18n("niixtorrent_description"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

    auto footerSizer = new wxBoxSizer(wxHORIZONTAL);
    footerSizer->Add(new wxStaticText(this, wxID_ANY, i18n("copyright_text")), 0, wxALIGN_CENTER_VERTICAL);
    footerSizer->AddStretchSpacer();
    footerSizer->Add(new wxHyperlinkCtrl(this, wxID_ANY, "torrent.niiix.net", "https://torrent.niiix.net"), 0, wxALIGN_CENTER_VERTICAL);

    auto buttons = CreateStdDialogButtonSizer(wxCLOSE);

    auto border = FromDIP(16);
    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(bmp, 0, wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER_HORIZONTAL, border);
    sizer->AddSpacer(FromDIP(8));
    sizer->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, border);
    sizer->AddSpacer(FromDIP(2));
    sizer->Add(version, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, border);
    sizer->AddSpacer(FromDIP(10));
    sizer->Add(desc, 0, wxEXPAND | wxLEFT | wxRIGHT, border);
    sizer->AddSpacer(FromDIP(12));
    sizer->Add(lv, wxSizerFlags(1).Border(wxLEFT | wxRIGHT, border).Expand());
    sizer->AddSpacer(FromDIP(12));
    sizer->Add(footerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, border);
    sizer->AddSpacer(FromDIP(12));
    sizer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, border);

    this->SetSizer(sizer);
    this->SetSize(FromDIP(wxSize(400, 560)));
    this->SetMinSize(FromDIP(wxSize(360, 480)));
    this->CentreOnParent();

    // Wrap the description to the dialog width once that is known.
    desc->Wrap(GetClientSize().GetWidth() - 2 * border);
    this->Layout();

    this->SetAffirmativeId(wxID_CLOSE);
    this->SetEscapeId(wxID_CLOSE);
}

AboutDialog::~AboutDialog()
{
}
