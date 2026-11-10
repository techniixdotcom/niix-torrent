#include "preferencesgeneralpage.hpp"

#include <boost/log/trivial.hpp>

#include "../clientdata.hpp"
#include "../../core/autostart.hpp"
#include "../../core/configuration.hpp"
#include "../../core/utils.hpp"
#include "../theme.hpp"

#include <wx/taskbar.h>
#include "../translator.hpp"

using pt::Core::Configuration;
using pt::UI::Dialogs::PreferencesGeneralPage;

PreferencesGeneralPage::PreferencesGeneralPage(wxWindow* parent, std::shared_ptr<pt::Core::Configuration> cfg)
    : wxPanel(parent),
    m_cfg(cfg)
{
    wxStaticBoxSizer* uiSizer = new wxStaticBoxSizer(wxVERTICAL, this, i18n("user_interface"));
    wxFlexGridSizer* uiGrid = new wxFlexGridSizer(2, FromDIP(10), FromDIP(10));
    m_language = new wxChoice(uiSizer->GetStaticBox(), wxID_ANY);
    uiGrid->AddGrowableCol(1, 1);
    uiGrid->Add(new wxStaticText(uiSizer->GetStaticBox(), wxID_ANY, i18n("language")), 0, wxALIGN_CENTER_VERTICAL);
    uiGrid->Add(m_language, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

    m_theme = new wxChoice(uiSizer->GetStaticBox(), wxID_ANY);
    uiGrid->Add(new wxStaticText(uiSizer->GetStaticBox(), wxID_ANY, i18n("theme")), 0, wxALIGN_CENTER_VERTICAL);
    uiGrid->Add(m_theme, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    uiSizer->Add(uiGrid, 1, wxEXPAND | wxALL, FromDIP(5));

    wxStaticBoxSizer* miscSizer = new wxStaticBoxSizer(wxVERTICAL, this, i18n("miscellaneous"));
    wxFlexGridSizer* miscGrid = new wxFlexGridSizer(2, FromDIP(10), FromDIP(10));

    m_labelColor = new wxCheckBox(miscSizer->GetStaticBox(), wxID_ANY, i18n("use_label_color_as_background_in_torrent_list"));
    m_skipAddTorrentDialog = new wxCheckBox(miscSizer->GetStaticBox(), wxID_ANY, i18n("skip_add_torrent_dialog"));
    m_autoStart = new wxCheckBox(miscSizer->GetStaticBox(), wxID_ANY, 
#ifdef _WIN32
        i18n("start_with_windows"));
#else
        i18n("start_at_login"));
#endif
    m_startPosition = new wxChoice(miscSizer->GetStaticBox(), wxID_ANY);

    miscGrid->AddGrowableCol(1, 1);
    miscGrid->Add(m_labelColor, 0, wxALIGN_CENTER_VERTICAL);
    miscGrid->Add(0, 0);
    miscGrid->Add(m_skipAddTorrentDialog, 0, wxALIGN_CENTER_VERTICAL);
    miscGrid->Add(0, 0);
    miscGrid->Add(m_autoStart, 0, wxALIGN_CENTER_VERTICAL);
    miscGrid->Add(0, 0);
    miscGrid->Add(new wxStaticText(miscSizer->GetStaticBox(), wxID_ANY, i18n("start_position")), 0, wxALIGN_CENTER_VERTICAL);
    miscGrid->Add(m_startPosition, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    miscSizer->Add(miscGrid, 1, wxEXPAND | wxALL, FromDIP(5));

    wxStaticBoxSizer* notifSizer = new wxStaticBoxSizer(wxVERTICAL, this, i18n("notification_area"));
    wxFlexGridSizer* notifGrid = new wxFlexGridSizer(1, FromDIP(10), FromDIP(10));

    m_showNotificationIcon = new wxCheckBox(notifSizer->GetStaticBox(), wxID_ANY, i18n("show_niixtorrent_in_notification_area"));
    m_minimizeNotification = new wxCheckBox(notifSizer->GetStaticBox(), wxID_ANY, i18n("minimize_to_notification_area"));
    m_closeNotification = new wxCheckBox(notifSizer->GetStaticBox(), wxID_ANY, i18n("close_to_notification_area"));

    notifGrid->AddGrowableCol(0, 1);
    notifGrid->Add(m_showNotificationIcon);
    notifGrid->Add(m_minimizeNotification, 0, wxLEFT, FromDIP(15));
    notifGrid->Add(m_closeNotification, 0, wxLEFT, FromDIP(15));
    notifSizer->Add(notifGrid, 1, wxEXPAND | wxALL, FromDIP(5));

    // Without a notification area (e.g. GNOME without the AppIndicator
    // extension) these options cannot work - say so instead of silently
    // ignoring them.
    if (!wxTaskBarIcon::IsAvailable())
    {
        auto hint = new wxStaticText(notifSizer->GetStaticBox(), wxID_ANY, i18n("notification_area_unavailable"));
        hint->SetForegroundColour(pt::UI::Theme::TextMuted());
        hint->Wrap(FromDIP(420));
        notifSizer->Add(hint, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));
    }

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(uiSizer, 0, wxEXPAND);
    sizer->AddSpacer(FromDIP(10));
    sizer->Add(miscSizer, 0, wxEXPAND);
    sizer->AddSpacer(FromDIP(10));
    sizer->Add(notifSizer, 0, wxEXPAND);
    sizer->AddStretchSpacer();

    this->SetSizerAndFit(sizer);

    for (auto& lang : Translator::GetInstance().Languages())
    {
        int pos = m_language->Append(lang.name, new ClientData<std::string>(lang.locale));

        if (lang.locale == Translator::GetInstance().GetLocale())
        {
            m_language->SetSelection(pos);
        }
    }

    // The system language has no translation: English is what is shown.
    if (m_language->GetSelection() == wxNOT_FOUND)
    {
        for (unsigned int i = 0; i < m_language->GetCount(); i++)
        {
            auto data = static_cast<ClientData<std::string>*>(m_language->GetClientObject(i));
            if (data->GetValue() == "en-US") { m_language->SetSelection(i); break; }
        }
    }

    m_initialLanguage = m_language->GetSelection();

    m_labelColor->SetValue(m_cfg->Get<bool>("use_label_as_list_bgcolor").value());
    m_skipAddTorrentDialog->SetValue(m_cfg->Get<bool>("skip_add_torrent_dialog").value());

    m_autoStart->SetValue(pt::Core::AutoStart::IsEnabled());

    // Theme
    m_theme->Append(i18n("follow_system_theme"), new ClientData<std::string>("system"));
    m_theme->Append(i18n("dark_theme"),   new ClientData<std::string>("dark"));
    m_theme->Append(i18n("light_theme"),  new ClientData<std::string>("light"));

    std::string themeId = m_cfg->Get<std::string>("theme_id").value_or("system");
    m_theme->SetSelection(themeId == "dark" ? 1 : (themeId == "light" ? 2 : 0));

    m_startPosition->Append(i18n("normal"), new ClientData<Configuration::WindowState>(Configuration::WindowState::Normal));
    m_startPosition->Append(i18n("minimized"), new ClientData<Configuration::WindowState>(Configuration::WindowState::Minimized));
    m_startPosition->Append(i18n("hidden"), new ClientData<Configuration::WindowState>(Configuration::WindowState::Hidden));
    m_startPosition->Append(i18n("maximized"), new ClientData<Configuration::WindowState>(Configuration::WindowState::Maximized));
    m_startPosition->SetSelection(m_cfg->Get<int>("start_position").value());

    // Notification area
    m_showNotificationIcon->SetValue(m_cfg->Get<bool>("show_in_notification_area").value());
    m_minimizeNotification->SetValue(m_cfg->Get<bool>("minimize_to_notification_area").value());
    m_closeNotification->SetValue(m_cfg->Get<bool>("close_to_notification_area").value());

    m_showNotificationIcon->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&)
        {
            m_minimizeNotification->Enable(m_showNotificationIcon->IsChecked());
            m_closeNotification->Enable(m_showNotificationIcon->IsChecked());
        });

    m_minimizeNotification->Enable(m_showNotificationIcon->IsChecked());
    m_closeNotification->Enable(m_showNotificationIcon->IsChecked());
}

PreferencesGeneralPage::~PreferencesGeneralPage()
{
}

bool PreferencesGeneralPage::IsValid()
{
    return true;
}

void PreferencesGeneralPage::Save(bool* restartRequired)
{
    int langIndex = m_language->GetSelection();
    ClientData<std::string>* langData = langIndex >= 0
        ? static_cast<ClientData<std::string>*>(m_language->GetClientObject(langIndex))
        : nullptr;

    int startPosIndex = m_startPosition->GetSelection();
    ClientData<Configuration::WindowState>* startPosData = static_cast<ClientData<Configuration::WindowState>*>(m_startPosition->GetClientObject(startPosIndex));

    if (langData != nullptr && langIndex != m_initialLanguage)
    {
        *restartRequired = true;
        m_cfg->Set("locale_name", langData->GetValue());
        m_initialLanguage = langIndex;
    }

    if (startPosData != nullptr)
    {
        m_cfg->Set("start_position", static_cast<int>(startPosData->GetValue()));
    }

    if (pt::Core::AutoStart::IsEnabled() != m_autoStart->GetValue())
    {
        pt::Core::AutoStart::SetEnabled(m_autoStart->GetValue());
    }

    int themeIndex = m_theme->GetSelection();

    if (themeIndex >= 0)
    {
        auto themeData = static_cast<ClientData<std::string>*>(m_theme->GetClientObject(themeIndex));
        std::string current = m_cfg->Get<std::string>("theme_id").value_or("system");

        if (themeData->GetValue() != current)
        {
            // Native controls pick their colours up at start-up.
            *restartRequired = true;
            m_cfg->Set("theme_id", themeData->GetValue());
        }
    }

    m_cfg->Set("use_label_as_list_bgcolor", m_labelColor->GetValue());
    m_cfg->Set("skip_add_torrent_dialog", m_skipAddTorrentDialog->GetValue());
    m_cfg->Set("show_in_notification_area", m_showNotificationIcon->GetValue());
    m_cfg->Set("minimize_to_notification_area", m_minimizeNotification->GetValue());
    m_cfg->Set("close_to_notification_area", m_closeNotification->GetValue());
}
