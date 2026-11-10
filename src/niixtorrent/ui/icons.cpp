#include "icons.hpp"

#include <wx/image.h>
#include <wx/mstream.h>

#include <cstring>

#include "../core/embeddedresources.hpp"

using pt::UI::Icons;

static const pt::Core::EmbeddedResource* Find(std::string const& name)
{
    for (auto res = pt::Core::EmbeddedIcons(); res->name != nullptr; ++res)
    {
        if (name == res->name) { return res; }
    }

    return nullptr;
}

static wxImage LoadPng(std::string const& name)
{
    if (wxImage::FindHandler(wxBITMAP_TYPE_PNG) == nullptr)
    {
        wxImage::AddHandler(new wxPNGHandler());
    }

    if (auto res = Find(name))
    {
        wxMemoryInputStream stream(res->data, res->size);
        wxImage image(stream, wxBITMAP_TYPE_PNG);
        if (image.IsOk()) { return image; }
    }

    return wxImage();
}

wxIconBundle const& Icons::App()
{
    static wxIconBundle bundle;

    if (bundle.IsEmpty())
    {
        for (int size : { 16, 24, 32, 48, 64, 128, 256 })
        {
            wxImage image = LoadPng("app-" + std::to_string(size));
            if (!image.IsOk()) { continue; }

            wxIcon icon;
            icon.CopyFromBitmap(wxBitmap(image));
            bundle.AddIcon(icon);
        }
    }

    return bundle;
}

wxBitmap Icons::AppBitmap(int size)
{
    // Pick the smallest embedded size that is large enough and scale down
    // with a high quality filter.
    int source = 256;

    for (int candidate : { 16, 24, 32, 48, 64, 128, 256 })
    {
        if (candidate >= size) { source = candidate; break; }
    }

    wxImage image = LoadPng("app-" + std::to_string(source));
    if (!image.IsOk()) { return wxNullBitmap; }

    if (source != size)
    {
        image.Rescale(size, size, wxIMAGE_QUALITY_HIGH);
    }

    return wxBitmap(image);
}

wxBitmapBundle Icons::Glyph(std::string const& name, wxSize const& size, wxColour const& colour)
{
    auto res = Find(name);
    if (res == nullptr) { return wxBitmapBundle(); }

    // The glyphs are drawn with "currentColor" so one file serves both themes.
    std::string svg(reinterpret_cast<const char*>(res->data), res->size);
    std::string hex = colour.GetAsString(wxC2S_HTML_SYNTAX).utf8_string();

    static const char* token = "currentColor";
    for (size_t pos = svg.find(token); pos != std::string::npos; pos = svg.find(token, pos + hex.size()))
    {
        svg.replace(pos, std::strlen(token), hex);
    }

#ifdef wxHAS_SVG
    return wxBitmapBundle::FromSVG(svg.c_str(), size);
#else
    (void)size;
    return wxBitmapBundle();
#endif
}
