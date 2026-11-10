#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/colour.h>
#include <wx/font.h>

#include <string>

namespace pt
{
namespace UI
{
    // The NiiX look (after niixaOS): one palette per appearance (dark / light). The theme is
    // selected once at start-up from the "theme_id" setting ("system", "dark"
    // or "light") - native controls take their colours when they are created.
    class Theme
    {
    public:
        enum class Mode
        {
            System,
            Dark,
            Light
        };

        // Must be called before the first window is created.
        static void Initialize(std::string const& themeId);

        static Mode CurrentMode();
        static bool IsDark();
        static bool IsSystemDark();

        // Surfaces
        static wxColour Background();   // window chrome, panels
        static wxColour Surface();      // lists, text fields
        static wxColour SurfaceAlt();   // zebra stripe in lists
        static wxColour Border();
        static wxColour Track();        // empty part of progress bars
        static wxColour Selection();    // selected row

        // Text
        static wxColour Text();
        static wxColour TextMuted();

        // Brand / state colours
        static wxColour Accent();       // pink - selected / on, downloading
        static wxColour OnAccent();     // text drawn on top of Accent()
        static wxColour Info();         // green - seeding, complete
        static wxColour Highlight();    // amber - checking
        static wxColour AccentEdge();   // thin pink hairline on the chrome
        static wxColour AccentTint();   // pink wash behind the active tab

        // Black or white, whichever reads better on `background`.
        static wxColour TextOn(wxColour const& background);
        static wxColour Error();
        static wxColour Paused();

        // Fixed-width font for the filter bar and hashes.
        static wxFont MonoFont(wxWindow const* window);

        // Applies Background()/Text() to a custom-drawn window.
        static void Apply(wxWindow* window);

        // Blends `fg` over `bg` with the given opacity (0..1).
        static wxColour Blend(wxColour const& fg, wxColour const& bg, double alpha);
    };
}
}
