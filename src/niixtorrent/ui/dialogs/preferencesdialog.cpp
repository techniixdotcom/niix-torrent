#include "preferencesdialog.hpp"

#include <wx/msgdlg.h>

#include <wx/bookctrl.h>
#include <wx/listbook.h>
#include <wx/listctrl.h>
#include <wx/hyperlink.h>
#include <wx/persist.h>
#include <wx/persist/toplevel.h>
#include <wx/simplebook.h>

#include "../../core/configuration.hpp"
#include "preferencesadvancedpage.hpp"
#include "preferencesconnectionpage.hpp"
#include "preferencesdownloadspage.hpp"
#include "preferencesgeneralpage.hpp"
#include "preferenceslabelspage.hpp"
#include "preferencesproxypage.hpp"
#include "../translator.hpp"
#include "../widgets/tabbook.hpp"

using pt::UI::Dialogs::PreferencesDialog;

PreferencesDialog::PreferencesDialog(wxWindow* parent, std::shared_ptr<pt::Core::Configuration> cfg)
    : wxDialog(parent, wxID_ANY, i18n("preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_book(new wxSimplebook(this, wxID_ANY)),
    m_general(new PreferencesGeneralPage(m_book, cfg)),
    m_downloads(new PreferencesDownloadsPage(m_book, cfg)),
    m_labels(new PreferencesLabelsPage(m_book, cfg)),
    m_connection(new PreferencesConnectionPage(m_book, cfg)),
    m_proxy(new PreferencesProxyPage(m_book, cfg)),
    m_advanced(new PreferencesAdvancedPage(m_book, cfg)),
    m_wantsRestart(false)
{
    m_list = new Widgets::TabBar(this, wxID_ANY, wxVERTICAL);
    m_list->AddTab(i18n("general"));
    m_list->AddTab(i18n("downloads"));
    m_list->AddTab(i18n("labels"));
    m_list->AddTab(i18n("connection"));
    m_list->AddTab(i18n("proxy"));
    m_list->AddTab(i18n("advanced"));
    m_list->SetSelection(0);

    m_book->AddPage(m_general, wxEmptyString, true);
    m_book->AddPage(m_downloads, wxEmptyString, false);
    m_book->AddPage(m_labels, wxEmptyString, false);
    m_book->AddPage(m_connection, wxEmptyString, false);
    m_book->AddPage(m_proxy, wxEmptyString, false);
    m_book->AddPage(m_advanced, wxEmptyString, false);

    m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
    m_mainSizer->Add(m_list, 0, wxEXPAND | wxRIGHT, FromDIP(12));
    m_mainSizer->Add(m_book, 1, wxEXPAND);

    auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxHyperlinkCtrl(this, wxID_ANY, i18n("documentation"), "https://github.com/techniixdotcom/niix-torrent"), 0, wxALIGN_CENTER_VERTICAL);
    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(new wxButton(this, ptID_BTN_RESTORE_DEFAULTS, i18n("restore_defaults")), 0, wxRIGHT, FromDIP(7));
    buttonSizer->Add(new wxButton(this, wxID_OK, i18n("ok")), 0, wxRIGHT, FromDIP(7));
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, i18n("cancel")));

    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_mainSizer, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(11));
    sizer->AddSpacer(FromDIP(7));
    sizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, FromDIP(11));

    this->SetSizerAndFit(sizer);

    if (!wxPersistenceManager::Get().RegisterAndRestore(this))
    {
        this->SetSize(FromDIP(wxSize(450, 400)));
    }

    this->Bind(wxEVT_BUTTON, &PreferencesDialog::OnOk, this, wxID_OK);
    this->Bind(
        wxEVT_CHOICE,
        [this](wxCommandEvent& evt)
        {
            m_book->ChangeSelection(evt.GetInt());
        },
        m_list->GetId());

    this->Bind(
        wxEVT_BUTTON,
        [this, cfg](wxCommandEvent&)
        {
            if (wxMessageBox(
                    i18n("restore_defaults_description"),
                    "NiiX Torrent",
                    wxICON_WARNING | wxYES_NO | wxNO_DEFAULT) == wxYES)
            {
                cfg->RestoreDefaults();
                this->EndDialog(wxID_OK);
            }
        },
        ptID_BTN_RESTORE_DEFAULTS);
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::OnOk(wxCommandEvent& evt)
{
    if (!m_general->IsValid())
    {
        return;
    }

    if (!m_downloads->IsValid())
    {
        return;
    }

    if (!m_labels->IsValid())
    {
        return;
    }

    if (!m_connection->IsValid())
    {
        return;
    }

    if (!m_proxy->IsValid())
    {
        return;
    }

    if (!m_advanced->IsValid())
    {
        return;
    }

    bool restartRequired = false;

    m_general->Save(&restartRequired);
    m_downloads->Save();
    m_labels->Save();
    m_connection->Save(&restartRequired);
    m_proxy->Save();
    m_advanced->Save();

    if (restartRequired)
    {
        ShowRestartRequiredDialog();
    }

    evt.Skip();
}

void PreferencesDialog::ShowRestartRequiredDialog()
{
    // wxMessageDialog is a task dialog on Windows and a native message
    // dialog elsewhere.
    wxMessageDialog dlg(
        this,
        i18n("prompt_restart"),
        i18n("prompt_restart_title"),
        wxOK | wxCANCEL | wxICON_INFORMATION);

    dlg.SetOKLabel(wxString(i18n("restart_niixtorrent")));

    m_wantsRestart = (dlg.ShowModal() == wxID_OK);
}
