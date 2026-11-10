#include "console.hpp"

#include "filters/pqltorrentfilter.hpp"
#include "ids.hpp"
#include "models/torrentlistmodel.hpp"
#include "torrentlistview.hpp"
#include "../core/configuration.hpp"
#include "icons.hpp"
#include "theme.hpp"
#include "translator.hpp"

using pt::UI::Console;
using pt::UI::Theme;

wxDEFINE_EVENT(ptEVT_FILTER_CHANGED, wxCommandEvent);

Console::Console(wxWindow* parent, wxWindowID id, pt::UI::Models::TorrentListModel* model)
    : wxPanel(parent, id),
    m_input(new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_PROCESS_ENTER)),
    m_model(model)
{
    Theme::Apply(this);

    m_input->SetFont(Theme::MonoFont(m_input));
    m_input->SetBackgroundColour(Theme::Surface());
    m_input->SetForegroundColour(Theme::Text());
    m_input->SetHint(i18n("filter_hint"));

    auto glyph = new wxStaticBitmap(
        this,
        wxID_ANY,
        Icons::Glyph("filter", wxSize(16, 16), Theme::TextMuted()));
    glyph->SetToolTip(i18n("filter_tooltip"));

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(glyph, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    sizer->Add(m_input, 1, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(6));

    this->SetSizerAndFit(sizer);

    this->Bind(
        wxEVT_TEXT_ENTER,
        [this](wxCommandEvent&)
        {
            CreateFilter(m_input->GetValue().utf8_string());
        });

    // Escape clears the filter.
    m_input->Bind(
        wxEVT_KEY_DOWN,
        [this](wxKeyEvent& evt)
        {
            if (evt.GetKeyCode() == WXK_ESCAPE && !m_input->IsEmpty())
            {
                m_input->Clear();
                CreateFilter(std::string());
                return;
            }

            evt.Skip();
        });
}

void Console::SetText(std::string const& text)
{
    m_input->ChangeValue(wxString::FromUTF8(text));
    CreateFilter(text);
}

void Console::CreateFilter(std::string const& input)
{
    if (input.empty())
    {
        m_model->ClearFilter();
    }
    else
    {
        std::string err;
        auto filter = Filters::PqlTorrentFilter::Create(input, &err);

        if (!filter && !err.empty())
        {
            wxMessageBox(wxString::FromUTF8(err), i18n("filter_error"), wxICON_ERROR | wxOK, GetParent());
            return;
        }

        m_model->SetFilter(std::move(filter));
    }

    wxCommandEvent evt(ptEVT_FILTER_CHANGED);
    evt.SetString(wxString::FromUTF8(input));

    wxPostEvent(GetParent(), evt);
}
