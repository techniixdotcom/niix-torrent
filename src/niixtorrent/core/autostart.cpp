#include "autostart.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <boost/log/trivial.hpp>

#include "utils.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdlib>
#include <gio/gio.h>
#endif

namespace fs = std::filesystem;
using pt::Core::AutoStart;

#ifdef _WIN32

static const wchar_t* RunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* RunValue = L"NiiXTorrent";

bool AutoStart::IsEnabled()
{
    return RegGetValueW(HKEY_CURRENT_USER, RunKey, RunValue, RRF_RT_REG_SZ, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
}

void AutoStart::SetEnabled(bool enabled)
{
    HKEY key = nullptr;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, RunKey, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
    {
        BOOST_LOG_TRIVIAL(warning) << "Could not open the Run registry key";
        return;
    }

    if (enabled)
    {
        std::wstring command = L"\"" + pt::Utils::executablePath().wstring() + L"\"";

        LSTATUS res = RegSetValueExW(
            key,
            RunValue,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));

        if (res != ERROR_SUCCESS)
        {
            BOOST_LOG_TRIVIAL(warning) << "NiiX Torrent could not be registered to run at start-up. Error: " << res;
        }
    }
    else
    {
        RegDeleteValueW(key, RunValue);
    }

    RegCloseKey(key);
}

#else

static const char* AppId = "io.github.niixtorrent.NiiXTorrent";

static bool IsFlatpak()
{
    std::error_code ec;
    return fs::exists("/.flatpak-info", ec);
}

static fs::path ConfigHome()
{
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] == '/')
    {
        return fs::path(xdg);
    }

    const char* home = std::getenv("HOME");
    return fs::path(home ? home : "/") / ".config";
}

// Our own marker so the checkbox reflects the last request made through the
// portal (the portal itself cannot be queried).
static fs::path FlatpakMarker()
{
    return ConfigHome() / "niixtorrent-autostart";
}

static fs::path DesktopEntry()
{
    return ConfigHome() / "autostart" / (std::string(AppId) + ".desktop");
}

static void RequestBackgroundPortal(bool enabled)
{
    GDBusConnection* bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
    if (bus == nullptr) { return; }

    GVariantBuilder commandline;
    g_variant_builder_init(&commandline, G_VARIANT_TYPE("as"));
    g_variant_builder_add(&commandline, "s", "niixtorrent");

    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "reason", g_variant_new_string("Start NiiX Torrent when you sign in"));
    g_variant_builder_add(&options, "{sv}", "autostart", g_variant_new_boolean(enabled));
    g_variant_builder_add(&options, "{sv}", "commandline", g_variant_builder_end(&commandline));

    GError* error = nullptr;
    GVariant* reply = g_dbus_connection_call_sync(
        bus,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Background",
        "RequestBackground",
        g_variant_new("(sa{sv})", "", &options),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        5000,
        nullptr,
        &error);

    if (error != nullptr)
    {
        BOOST_LOG_TRIVIAL(warning) << "Background portal request failed: " << error->message;
        g_error_free(error);
    }

    if (reply != nullptr) { g_variant_unref(reply); }
    g_object_unref(bus);
}

bool AutoStart::IsEnabled()
{
    std::error_code ec;
    return fs::exists(IsFlatpak() ? FlatpakMarker() : DesktopEntry(), ec);
}

void AutoStart::SetEnabled(bool enabled)
{
    std::error_code ec;

    if (IsFlatpak())
    {
        RequestBackgroundPortal(enabled);

        if (enabled) { std::ofstream(FlatpakMarker()) << "1\n"; }
        else { fs::remove(FlatpakMarker(), ec); }

        return;
    }

    fs::path entry = DesktopEntry();

    if (!enabled)
    {
        fs::remove(entry, ec);
        return;
    }

    fs::create_directories(entry.parent_path(), ec);

    std::ofstream out(entry, std::ios::trunc);
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=NiiX Torrent\n"
        << "Exec=\"" << pt::Utils::toUtf8(pt::Utils::executablePath()) << "\"\n"
        << "Icon=" << AppId << "\n"
        << "Terminal=false\n"
        << "X-GNOME-Autostart-enabled=true\n";

    if (!out)
    {
        BOOST_LOG_TRIVIAL(warning) << "Could not write " << entry;
    }
}

#endif
