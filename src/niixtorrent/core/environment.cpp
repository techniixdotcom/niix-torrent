#include "environment.hpp"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>
#include <wx/uilocale.h>

#include <cstdlib>
#include <ctime>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>
#include <Shlwapi.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "../buildinfo.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;
using pt::Core::Environment;

namespace expr = boost::log::expressions;

Environment::Environment()
    : m_startupTime(std::chrono::system_clock::now())
{
}

std::shared_ptr<Environment> Environment::Create()
{
    auto env = new Environment();

    boost::log::add_file_log
    (
        boost::log::keywords::file_name = env->GetLogFilePath(),
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = expr::stream
            << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") << " "
            << "[" << expr::attr<boost::log::attributes::timer::value_type>("Uptime") << "] "
            << "[" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID") << "] "
            << boost::log::trivial::severity << ": "
            << expr::message
    );

    boost::log::add_common_attributes();
    boost::log::core::get()->add_global_attribute("Uptime", boost::log::attributes::timer());

    BOOST_LOG_TRIVIAL(info) << "NiiX Torrent " << pt::BuildInfo::version() << " (" << pt::BuildInfo::commitish() << ") starting up...";

    return std::shared_ptr<Environment>(env);
}

fs::path Environment::GetApplicationDataPath()
{
#ifdef _WIN32
    if (IsInstalled() || IsAppContainerProcess())
    {
        return fs::path(GetKnownFolderPath(KnownFolder::LocalAppData)) / "NiiX Torrent";
    }

    // Portable mode: keep everything next to the executable.
    return GetApplicationPath();
#else
    // $XDG_DATA_HOME/niixtorrent (~/.local/share/niixtorrent, or the
    // per-application data directory when running as a Flatpak).
    fs::path dataDir = GetKnownFolderPath(KnownFolder::LocalAppData) / "niixtorrent";

    std::error_code ec;
    fs::create_directories(dataDir, ec);

    return dataDir;
#endif
}

fs::path Environment::GetApplicationPath()
{
    return Utils::executablePath().parent_path();
}

std::string Environment::GetCrashpadReportUrl()
{
    if (const char* url = std::getenv("NIIXTORRENT_CRASHPAD_URL"))
    {
        return url;
    }

    // Empty unless a collector was configured at build time (NIIX_CRASHPAD_URL).
    return pt::BuildInfo::crashpadUrl();
}

fs::path Environment::GetCoreDbFilePath()
{
    std::error_code ec;

    // Next to the executable: Windows installs, portable builds and running
    // straight from the build tree.
    fs::path local = GetApplicationPath() / "coredb.sqlite";
    if (fs::exists(local, ec)) { return local; }

#ifndef _WIN32
    // Relocatable install: <prefix>/bin/niixtorrent -> <prefix>/share/niixtorrent
    fs::path relative = GetApplicationPath().parent_path() / "share" / "niixtorrent" / "coredb.sqlite";
    if (fs::exists(relative, ec)) { return relative; }

    return fs::path(NIIX_DATA_DIR) / "coredb.sqlite";
#else
    return local;
#endif
}

std::string Environment::GetCurrentLocale()
{
    // BCP 47 tag of the user's preferred UI language, e.g. "en-US" or "sv-SE".
    wxString tag = wxUILocale::GetSystemLocaleId().GetTag(wxLOCALE_TAGTYPE_BCP47);

    if (tag.empty())
    {
        BOOST_LOG_TRIVIAL(error) << "Failed to get current locale - defaulting to 'en'";
        return "en";
    }

    return tag.utf8_string();
}

fs::path Environment::GetDatabaseFilePath()
{
    return GetApplicationDataPath() / "NiiXTorrent.sqlite";
}

fs::path Environment::GetKnownFolderPath(Environment::KnownFolder knownFolder)
{
    wxStandardPaths& paths = wxStandardPaths::Get();
    wxString result;

    switch (knownFolder)
    {
    case KnownFolder::LocalAppData:
#ifdef _WIN32
        // %LOCALAPPDATA%
        result = paths.GetUserDir(wxStandardPaths::Dir_Cache);
#else
        if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg != nullptr && xdg[0] == '/')
        {
            return Utils::fromUtf8(xdg);
        }
        return Utils::fromUtf8(wxGetHomeDir().utf8_string()) / ".local" / "share";
#endif
        break;

    case KnownFolder::UserDownloads:
        result = paths.GetUserDir(wxStandardPaths::Dir_Downloads);

#ifndef _WIN32
        // Without xdg-user-dirs wxWidgets answers with the home directory.
        if (result.empty() || wxFileName(result, "").SameAs(wxFileName(wxGetHomeDir(), "")))
        {
            result = wxGetHomeDir() + "/Downloads";
        }
#endif
        break;

    default:
        BOOST_LOG_TRIVIAL(fatal) << "Unknown KnownFolder specified: " << static_cast<int>(knownFolder);
        throw std::runtime_error("Unknown folder");
    }

    if (result.empty())
    {
        BOOST_LOG_TRIVIAL(fatal) << "Failed to get KnownFolder: " << static_cast<int>(knownFolder);
        throw std::runtime_error("Could not get known folder path");
    }

    return fs::path(result.ToStdWstring());
}

fs::path Environment::GetLogFilePath()
{
    std::time_t tim = std::chrono::system_clock::to_time_t(m_startupTime);
    std::tm t = {};

#ifdef _WIN32
    localtime_s(&t, &tim);
#else
    localtime_r(&tim, &t);
#endif

    char frmt[100] = { 0 };
    snprintf(frmt,
        sizeof(frmt),
        "NiiXTorrent.%d%02d%02d%02d%02d%02d.log",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec);

    return GetApplicationDataPath() / "logs" / frmt;
}

bool Environment::IsAppContainerProcess()
{
#ifdef _WIN32
    std::error_code ec;
    return fs::is_regular_file(GetApplicationPath() / "appx.dummy", ec);
#else
    return false;
#endif
}

#ifdef _WIN32
static bool IsInstalledIn(HKEY root, fs::path const& executable)
{
    // Written by the installer (packaging/windows/NiiXTorrent.iss). Per-machine
    // installs use HKLM, per-user installs HKCU.
    wchar_t installDirectory[MAX_PATH] = { 0 };
    DWORD bufSize = sizeof(installDirectory);

    LSTATUS status = RegGetValueW(
        root,
        L"Software\\NiiXTorrent",
        L"InstallDirectory",
        RRF_RT_REG_SZ,
        nullptr,
        installDirectory,
        &bufSize);

    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    std::error_code ec;
    fs::path installed = fs::path(installDirectory) / "NiiXTorrent.exe";

    return fs::equivalent(installed, executable, ec) && !ec;
}
#endif

bool Environment::IsInstalled()
{
#ifdef _WIN32
    fs::path executable = Utils::executablePath();

    return IsInstalledIn(HKEY_LOCAL_MACHINE, executable)
        || IsInstalledIn(HKEY_CURRENT_USER, executable);
#else
    return true;
#endif
}
