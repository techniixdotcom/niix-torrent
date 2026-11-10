#include "utils.hpp"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/strconv.h>
#include <wx/utils.h>

#include <array>
#include <cmath>
#include <system_error>

#include <fmt/format.h>
#include <fmt/xchar.h>

#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#else
#include <gio/gio.h>
#endif

namespace fs = std::filesystem;
using pt::Utils;

void Utils::openAndSelect(fs::path path)
{
#ifdef _WIN32
    // SHOpenFolderAndSelectItems reuses an already open Explorer window and
    // copes with paths that contain commas or spaces, unlike "explorer /select".
    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(path.native().c_str());

    if (pidl != nullptr)
    {
        SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
        ILFree(pidl);
        return;
    }

    wxLaunchDefaultApplication(path.parent_path().wstring());
#else
    // org.freedesktop.FileManager1 is implemented by Nautilus, Dolphin, Nemo,
    // Thunar, ... and selects the item. Fall back to opening the parent folder.
    bool shown = false;
    std::string uri = "file://" + toUtf8(path);

    if (gchar* escaped = g_filename_to_uri(toUtf8(path).c_str(), nullptr, nullptr))
    {
        uri = escaped;
        g_free(escaped);
    }

    if (GDBusConnection* bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr))
    {
        GVariantBuilder uris;
        g_variant_builder_init(&uris, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&uris, "s", uri.c_str());

        GError* error = nullptr;
        GVariant* reply = g_dbus_connection_call_sync(
            bus,
            "org.freedesktop.FileManager1",
            "/org/freedesktop/FileManager1",
            "org.freedesktop.FileManager1",
            "ShowItems",
            g_variant_new("(ass)", &uris, ""),
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            2000,
            nullptr,
            &error);

        if (reply != nullptr)
        {
            shown = true;
            g_variant_unref(reply);
        }

        if (error != nullptr) { g_error_free(error); }
        g_object_unref(bus);
    }

    if (!shown)
    {
        std::error_code ec;
        fs::path folder = fs::is_directory(path, ec) ? path : path.parent_path();
        wxLaunchDefaultApplication(wxString::FromUTF8(toUtf8(folder)));
    }
#endif
}

std::wstring Utils::toHumanFileSize(int64_t bytes)
{
    static const std::array<const wchar_t*, 6> units = { L"bytes", L"KB", L"MB", L"GB", L"TB", L"PB" };

    if (bytes < 0) { bytes = 0; }

    if (bytes < 1024)
    {
        return fmt::format(L"{} {}", bytes, units[0]);
    }

    double value = static_cast<double>(bytes);
    size_t unit = 0;

    while (value >= 1024.0 && unit < units.size() - 1)
    {
        value /= 1024.0;
        unit++;
    }

    // Three significant digits, as Explorer does: 1.42 GB, 25.0 MB, 349 MB.
    int decimals = value >= 100.0 ? 0 : (value >= 10.0 ? 1 : 2);

    return fmt::format(L"{:.{}f} {}", value, decimals, units[unit]);
}

std::string Utils::toStdString(std::wstring const& input)
{
    return wxString(input).utf8_string();
}

std::wstring Utils::toStdWString(std::string const& input)
{
    return wxString::FromUTF8(input).ToStdWstring();
}

std::string Utils::toUtf8(fs::path const& path)
{
    auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
}

fs::path Utils::fromUtf8(std::string const& utf8)
{
    return fs::path(std::u8string(utf8.begin(), utf8.end()));
}

fs::path Utils::executablePath()
{
    return fs::path(wxStandardPaths::Get().GetExecutablePath().ToStdWstring());
}

double Utils::freeDiskSpaceRatio(fs::path const& path)
{
    std::error_code ec;
    fs::space_info info = fs::space(path, ec);

    if (ec || info.capacity == 0 || info.capacity == static_cast<std::uintmax_t>(-1))
    {
        return -1.0;
    }

    return static_cast<double>(info.available) / static_cast<double>(info.capacity);
}
