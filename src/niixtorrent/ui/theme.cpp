#include "theme.hpp"

#include <wx/fontenum.h>
#include <wx/settings.h>

#ifdef __WXMSW__
#include <wx/msw/darkmode.h>
#endif

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

using pt::UI::Theme;

namespace
{
    struct Palette
    {
        wxColour background;
        wxColour surface;
        wxColour surfaceAlt;
        wxColour border;
        wxColour track;
        wxColour selection;
        wxColour text;
        wxColour textMuted;
        wxColour accent;
        wxColour onAccent;
        wxColour info;
        wxColour highlight;
        wxColour error;
        wxColour paused;
    };

    // NiiX dark - the niixaOS look: black chrome, dark glass panels
    // (#121218), thin hairlines, and pink #ff0a6c for whatever is selected or
    // on. Green, amber and red appear only where they carry a state
    // (seeding, checking, error).
    const Palette& DarkPalette()
    {
        static const Palette p =
        {
            wxColour(0x07, 0x07, 0x0a), // background  - chrome: menu, tabs, filter and status bar
            wxColour(0x12, 0x12, 0x18), // surface     - panel: lists, inputs
            wxColour(0x16, 0x16, 0x1d), // surfaceAlt  - zebra stripe (white 2 %)
            wxColour(0x23, 0x23, 0x28), // border      - hairline (white 7 %)
            wxColour(0x2a, 0x2a, 0x2f), // track       - empty progress (white 10 %)
            wxColour(0x38, 0x11, 0x25), // selection   - pink 16 % over panel
            wxColour(0xf2, 0xf2, 0xf5), // text
            wxColour(0xa0, 0xa0, 0xae), // textMuted   - "dim"
            wxColour(0xff, 0x0a, 0x6c), // accent      - pink
            wxColour(0xff, 0xff, 0xff), // onAccent
            wxColour(0x0a, 0xff, 0x9e), // info        - green: seeding / complete
            wxColour(0xff, 0xd1, 0x66), // highlight   - amber: checking
            wxColour(0xff, 0x3b, 0x6b), // error       - red
            wxColour(0x5a, 0x5a, 0x68), // paused      - "mute"
        };
        return p;
    }

    // NiiX light: the same language on white - pink for selection and the
    // active tab, pink-tinted hairlines on the chrome, the state colours
    // deepened so they keep their contrast.
    const Palette& LightPalette()
    {
        static const Palette p =
        {
            wxColour(0xf4, 0xf4, 0xf7), // background
            wxColour(0xff, 0xff, 0xff), // surface
            wxColour(0xf9, 0xf9, 0xfb), // surfaceAlt
            wxColour(0xe3, 0xe3, 0xea), // border
            wxColour(0xea, 0xea, 0xef), // track
            wxColour(0xfb, 0xe6, 0xef), // selection   - pink 10 % over white
            wxColour(0x12, 0x12, 0x18), // text
            wxColour(0x5a, 0x5a, 0x68), // textMuted
            wxColour(0xd9, 0x00, 0x5b), // accent      - pink, deepened for white
            wxColour(0xff, 0xff, 0xff), // onAccent
            wxColour(0x00, 0x9e, 0x63), // info        - green
            wxColour(0xb2, 0x6a, 0x00), // highlight   - amber
            wxColour(0xd2, 0x1f, 0x4a), // error
            wxColour(0x9a, 0x9a, 0xa8), // paused
        };
        return p;
    }

    Theme::Mode g_mode = Theme::Mode::System;
    bool g_dark = true;

    const Palette& Current()
    {
        return Theme::IsDark() ? DarkPalette() : LightPalette();
    }

#ifdef __WXMSW__
    // Makes the native Windows controls use the NiiX dark palette instead of
    // the stock grey dark mode colours.
    class NiiXDarkModeSettings : public wxDarkModeSettings
    {
    public:
        wxColour GetColour(wxSystemColour index) override
        {
            const Palette& p = DarkPalette();

            switch (index)
            {
            case wxSYS_COLOUR_WINDOW:
            case wxSYS_COLOUR_LISTBOX:
                return p.surface;

            case wxSYS_COLOUR_BTNFACE:
            case wxSYS_COLOUR_MENU:
            case wxSYS_COLOUR_MENUBAR:
            case wxSYS_COLOUR_ACTIVECAPTION:
            case wxSYS_COLOUR_APPWORKSPACE:
            case wxSYS_COLOUR_INFOBK:
                return p.background;

            case wxSYS_COLOUR_WINDOWTEXT:
            case wxSYS_COLOUR_CAPTIONTEXT:
            case wxSYS_COLOUR_BTNTEXT:
            case wxSYS_COLOUR_MENUTEXT:
            case wxSYS_COLOUR_INFOTEXT:
            case wxSYS_COLOUR_LISTBOXTEXT:
            case wxSYS_COLOUR_HIGHLIGHTTEXT:
            case wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT:
                return p.text;

            case wxSYS_COLOUR_GRAYTEXT:
            case wxSYS_COLOUR_INACTIVECAPTIONTEXT:
                return p.textMuted;

            case wxSYS_COLOUR_HIGHLIGHT:
            case wxSYS_COLOUR_MENUHILIGHT:
                return p.selection;

            case wxSYS_COLOUR_HOTLIGHT:
                return p.info;

            case wxSYS_COLOUR_BTNSHADOW:
            case wxSYS_COLOUR_3DDKSHADOW:
            case wxSYS_COLOUR_3DLIGHT:
            case wxSYS_COLOUR_BTNHIGHLIGHT:
            case wxSYS_COLOUR_WINDOWFRAME:
            case wxSYS_COLOUR_ACTIVEBORDER:
            case wxSYS_COLOUR_INACTIVEBORDER:
            case wxSYS_COLOUR_INACTIVECAPTION:
            case wxSYS_COLOUR_SCROLLBAR:
                return p.border;

            default:
                return wxDarkModeSettings::GetColour(index);
            }
        }

        wxColour GetMenuColour(wxMenuColour which) override
        {
            const Palette& p = DarkPalette();

            switch (which)
            {
            case wxMenuColour::StandardFg: return p.text;
            case wxMenuColour::StandardBg: return p.background;
            case wxMenuColour::DisabledFg: return p.textMuted;
            case wxMenuColour::HotBg:      return p.selection;
            default:                       return wxDarkModeSettings::GetMenuColour(which);
            }
        }

        wxPen GetBorderPen() override
        {
            return wxPen(DarkPalette().border);
        }
    };
#endif

#ifdef __WXGTK__
    wxString Hex(wxColour const& c)
    {
        return c.GetAsString(wxC2S_HTML_SYNTAX);
    }

    // GTK widgets are styled by the desktop theme; this tints it towards the
    // NiiX palette so both platforms look alike.
    void InstallGtkPalette(bool dark)
    {
        const Palette& p = dark ? DarkPalette() : LightPalette();

        // Placeholders: @bg@ @surface@ @alt@ @border@ @sel@ @text@ @muted@ @accent@ @onaccent@ @info@ @hover@
        wxString css;

        // Both themes: accent colour for selection, focus and checked state.
        css +=
            "treeview.view:selected, treeview.view:selected:focus, entry selection, textview text selection,"
            "  menu menuitem:hover, menubar > menuitem:hover, popover modelbutton:hover"
            "  { background-color: @sel@; color: @text@; box-shadow: none; }"
            "entry:focus, spinbutton:focus { border-color: @accent@; box-shadow: inset 0 0 0 1px @accent@; }"
            "check:checked, radio:checked { background-image: none; background-color: @accent@; border-color: @accent@; color: @onaccent@; }"
            "*:link, link { color: @accent@; }"
            "button.link, button.link:hover, button.link:active { background: none; border-color: transparent; box-shadow: none; padding-left: 0; padding-right: 0; }"
            "button.link label, button.link:visited label, *:visited { color: @accent@; }"
            "button.link:hover label { color: @text@; }";

        if (dark)
        {
            css +=
                "@define-color theme_bg_color @bg@;"
                "@define-color theme_base_color @surface@;"
                "@define-color theme_fg_color @text@;"
                "@define-color theme_text_color @text@;"
                "@define-color theme_selected_bg_color @sel@;"
                "@define-color theme_selected_fg_color @text@;"
                "@define-color borders @border@;"
                "window, dialog, .background { background-color: @bg@; color: @text@; }"
                "menubar, statusbar, toolbar { background-color: @bg@; color: @text@; }"
                "menu, .menu, .context-menu, popover, popover.background { background-color: @alt@; color: @text@; border: 1px solid @border@; }"
                "menu separator { background-color: @border@; }"
                "menu menuitem:disabled label { color: @muted@; }"
                "treeview.view, textview text, list, iconview { background-color: @surface@; color: @text@; }"
                "treeview.view header button { background-color: @bg@; background-image: none; color: @muted@;"
                "  border-style: none none solid none; border-width: 1px; border-color: @border@; box-shadow: none; text-shadow: none; }"
                "treeview.view header button:hover { color: @text@; background-color: @hover@; }"
                "entry, spinbutton, spinbutton entry { background-color: @surface@; background-image: none; color: @text@;"
                "  border-color: @border@; box-shadow: none; caret-color: @accent@; }"
                "entry:disabled, spinbutton:disabled { color: @muted@; background-color: @bg@; }"
                "button, combobox button.combo { background-image: none; background-color: @hover@; color: @text@;"
                "  border-color: @border@; box-shadow: none; text-shadow: none; }"
                "button:hover, combobox button.combo:hover { background-color: shade(@hover@, 1.25); }"
                "button:active, button:checked { background-color: shade(@hover@, 0.85); }"
                "button:disabled { color: @muted@; background-color: @bg@; }"
                "button.default, button.suggested-action { border-color: @edge@; color: @accent@; }"
                "check, radio { background-image: none; background-color: @surface@; border-color: shade(@border@, 1.4); box-shadow: none; }"
                "scrollbar, scrollbar trough { background-color: transparent; border-color: transparent; }"
                "scrollbar slider { background-color: shade(@border@, 1.5); border-color: transparent; }"
                "scrollbar slider:hover { background-color: @muted@; }"
                "paned > separator { background-color: @border@; background-image: none; }"
                "frame > border, scrolledwindow, .frame { border-color: @border@; }"
                "label:disabled { color: @muted@; }"
                "tooltip, tooltip.background { background-color: @alt@; color: @text@; border: 1px solid @border@; }";
        }

        css.Replace("@bg@", Hex(p.background));
        css.Replace("@surface@", Hex(p.surface));
        css.Replace("@alt@", Hex(Theme::Blend(p.text, p.background, 0.03)));
        css.Replace("@border@", Hex(p.border));
        css.Replace("@sel@", Hex(p.selection));
        css.Replace("@text@", Hex(p.text));
        css.Replace("@muted@", Hex(p.textMuted));
        css.Replace("@accent@", Hex(p.accent));
        css.Replace("@onaccent@", Hex(p.onAccent));
        css.Replace("@info@", Hex(p.info));
        css.Replace("@edge@", Hex(Theme::Blend(p.accent, p.background, 0.6)));
        css.Replace("@hover@", Hex(Theme::Blend(p.text, p.background, 0.07)));

        GtkCssProvider* provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, css.utf8_str(), -1, nullptr);

        if (GdkScreen* screen = gdk_screen_get_default())
        {
            gtk_style_context_add_provider_for_screen(
                screen,
                GTK_STYLE_PROVIDER(provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }

        g_object_unref(provider);
    }
#endif
}

void Theme::Initialize(std::string const& themeId)
{
    if (themeId == "dark")       { g_mode = Mode::Dark; }
    else if (themeId == "light") { g_mode = Mode::Light; }
    else                         { g_mode = Mode::System; }

#ifdef __WXMSW__
    // wxWidgets 3.3 native dark mode, recoloured with the NiiX palette.
    switch (g_mode)
    {
    case Mode::Dark:
        wxTheApp->MSWEnableDarkMode(wxApp::DarkMode_Always, new NiiXDarkModeSettings());
        break;
    case Mode::System:
        wxTheApp->MSWEnableDarkMode(wxApp::DarkMode_Auto, new NiiXDarkModeSettings());
        break;
    case Mode::Light:
        break;
    }
#else
    switch (g_mode)
    {
    case Mode::Dark:   wxTheApp->SetAppearance(wxApp::Appearance::Dark);   break;
    case Mode::Light:  wxTheApp->SetAppearance(wxApp::Appearance::Light);  break;
    case Mode::System: wxTheApp->SetAppearance(wxApp::Appearance::System); break;
    }
#endif

    // Resolved once: native controls keep the colours they were created with,
    // and this is queried for every custom-drawn cell.
    switch (g_mode)
    {
    case Mode::Dark:   g_dark = true;  break;
    case Mode::Light:  g_dark = false; break;
    case Mode::System: g_dark = wxSystemSettings::GetAppearance().IsDark(); break;
    }

#ifdef __WXGTK__
    InstallGtkPalette(g_dark);
#endif
}

Theme::Mode Theme::CurrentMode()
{
    return g_mode;
}

bool Theme::IsSystemDark()
{
    return wxSystemSettings::GetAppearance().IsSystemDark();
}

bool Theme::IsDark()
{
    return g_dark;
}

wxColour Theme::Background() { return Current().background; }
wxColour Theme::Surface()    { return Current().surface; }
wxColour Theme::SurfaceAlt() { return Current().surfaceAlt; }
wxColour Theme::Border()     { return Current().border; }
wxColour Theme::Track()      { return Current().track; }
wxColour Theme::Selection()  { return Current().selection; }
wxColour Theme::Text()       { return Current().text; }
wxColour Theme::TextMuted()  { return Current().textMuted; }
wxColour Theme::Accent()     { return Current().accent; }
wxColour Theme::OnAccent()   { return Current().onAccent; }
wxColour Theme::Info()       { return Current().info; }
wxColour Theme::Highlight()  { return Current().highlight; }
wxColour Theme::Error()      { return Current().error; }
wxColour Theme::Paused()     { return Current().paused; }

wxFont Theme::MonoFont(wxWindow const* window)
{
    wxFont font = window != nullptr
        ? window->GetFont()
        : wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

    // Prefer JetBrains Mono when the user has it, otherwise the platform's
    // standard fixed-width font - never a silent fallback to a proportional one.
    static const wxString preferred[] = { "JetBrains Mono", "Cascadia Mono", "Consolas", "DejaVu Sans Mono", "Liberation Mono" };

    for (auto const& face : preferred)
    {
        if (wxFontEnumerator::IsValidFacename(face))
        {
            font.SetFaceName(face);
            return font;
        }
    }

    font.SetFamily(wxFONTFAMILY_TELETYPE);
    return font;
}

void Theme::Apply(wxWindow* window)
{
    if (window == nullptr) { return; }

    window->SetBackgroundColour(Background());
    window->SetForegroundColour(Text());
}

wxColour Theme::AccentEdge() { return Blend(Accent(), Background(), IsDark() ? 0.35 : 0.30); }
wxColour Theme::AccentTint() { return Blend(Accent(), Background(), IsDark() ? 0.18 : 0.10); }

wxColour Theme::TextOn(wxColour const& background)
{
    // Relative luminance (sRGB approximation is enough for a label).
    double l = (0.2126 * background.Red() + 0.7152 * background.Green() + 0.0722 * background.Blue()) / 255.0;
    return l > 0.6 ? wxColour(0x12, 0x12, 0x18) : wxColour(0xff, 0xff, 0xff);
}

wxColour Theme::Blend(wxColour const& fg, wxColour const& bg, double alpha)
{
    auto mix = [alpha](unsigned char f, unsigned char b)
    {
        return static_cast<unsigned char>(f * alpha + b * (1.0 - alpha) + 0.5);
    };

    return wxColour(
        mix(fg.Red(), bg.Red()),
        mix(fg.Green(), bg.Green()),
        mix(fg.Blue(), bg.Blue()));
}
