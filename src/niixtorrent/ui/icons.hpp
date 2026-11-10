#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/bmpbndl.h>
#include <wx/iconbndl.h>

#include <string>

namespace pt
{
namespace UI
{
    // Artwork compiled into the executable from res/icons (see
    // cmake/EmbedResources.cmake), so it is identical on every platform and
    // sharp at any DPI scale.
    class Icons
    {
    public:
        // The application icon in every available size (window, task bar, tray).
        static wxIconBundle const& App();

        // The application icon as a bitmap of (close to) the given size in pixels.
        static wxBitmap AppBitmap(int size);

        // A monochrome SVG glyph ("filter", "folder", "file") tinted with
        // `colour`; `size` is in DIPs.
        static wxBitmapBundle Glyph(std::string const& name, wxSize const& size, wxColour const& colour);
    };
}
}
