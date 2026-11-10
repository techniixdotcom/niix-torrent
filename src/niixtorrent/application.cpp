#include "application.hpp"

#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/filesys.h>
#include <wx/ipc.h>
#include <wx/log.h>
#include <wx/utils.h>
#include <wx/persist.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>
#include <wx/taskbarbutton.h>

#include <chrono>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#else
#include <cerrno>
#include <signal.h>
#include <sys/types.h>
#endif

#include "api/libniix_impl.hpp"
#include "crashpadinitializer.hpp"
#include "persistencemanager.hpp"
#include "core/configuration.hpp"
#include "core/database.hpp"
#include "core/environment.hpp"
#include "core/utils.hpp"
#include "ipc/server.hpp"
#include "ui/mainframe.hpp"
#include "ui/theme.hpp"
#include "ui/translator.hpp"

using json = nlohmann::json;
using pt::Application;

Application::Application()
    : wxApp()
{
    // The lock is created in OnInit: wxSingleInstanceChecker needs the
    // application object to exist on Unix.
    // DPI awareness (per-monitor v2) is declared in application.manifest.
    SetAppName("niixtorrent");
    SetAppDisplayName("NiiX Torrent");
    SetVendorName("techniix");

#ifdef __WXGTK__
    // Ties the windows to the installed .desktop file (icon, grouping) on
    // both X11 and Wayland.
    SetClassName("io.github.niixtorrent.NiiXTorrent");
#endif
}

Application::~Application()
{
    for (auto plugin : m_plugins)
    {
        delete plugin;
    }
}

bool Application::OnCmdLineParsed(wxCmdLineParser& parser)
{
    long waitForPid = -1;
    wxString save_path = "";

    if (parser.Found("wait-for-pid", &waitForPid))
    {
        m_options.pid = waitForPid;
    }

    m_options.silent = parser.Found("silent");

    if (parser.Found("save-path", &save_path))
    {
        m_options.save_path = save_path.utf8_string();
    }

    for (size_t i = 0; i < parser.GetParamCount(); i++)
    {
        std::string arg = parser.GetParam(i).utf8_string();

        if (arg.rfind("magnet:?xt", 0) == 0)
        {
            m_options.magnets.push_back(arg);
        }
        else
        {
            // Browsers hand over file:// URLs on Linux.
            if (arg.rfind("file://", 0) == 0)
            {
                wxFileName fn = wxFileSystem::URLToFileName(wxString::FromUTF8(arg));
                arg = fn.GetFullPath().utf8_string();
            }

            m_options.files.push_back(
                Utils::toUtf8(std::filesystem::absolute(Utils::fromUtf8(arg))));
        }
    }

    return true;
}

bool Application::OnInit()
{
    if (!wxApp::OnInit()) { return false; }

    if (m_options.pid > 0)
    {
        WaitForPreviousInstance(m_options.pid);
    }

#ifdef _WIN32
    m_singleInstance = std::make_unique<wxSingleInstanceChecker>("584c8e47-d8a5-4e52-9165-c0650a85723a");
#else
    m_singleInstance = std::make_unique<wxSingleInstanceChecker>(
        wxString::Format("niixtorrent-%s.lock", wxGetUserId()),
        pt::IPC::Server::RuntimeDirectory());
#endif

    if (m_singleInstance->IsAnotherRunning())
    {
        // Handing the arguments to the running instance is a success, not
        // an error: leave with exit code 0 (see OnRun) so launchers and
        // browsers do not report a failed start.
        ActivateOtherInstance();
        m_handedOver = true;
        return true;
    }

    auto env = pt::Core::Environment::Create();
    pt::CrashpadInitializer::Initialize(env);

    auto db = std::make_shared<pt::Core::Database>(env);


    if (!db->Migrate())
    {
        wxMessageBox(
            "Failed to run database migrations. Please check log file.",
            "NiiX Torrent",
            wxICON_ERROR);
        return false;
    }

    auto cfg = std::make_shared<pt::Core::Configuration>(db);

    // Load current locale
    pt::UI::Translator& translator = pt::UI::Translator::GetInstance();
    translator.LoadDatabase(env->GetCoreDbFilePath());
    translator.SetLocale(
        cfg->Get<std::string>("locale_name")
            .value_or(env->GetCurrentLocale()));
    
    // Theme: "system" (default), "dark" or "light". Has to be decided before
    // the first window exists.
    pt::UI::Theme::Initialize(
        cfg->Get<std::string>("theme_id").value_or("system"));

    LoadPlugins(env.get(), cfg.get());

    // Set up persistence manager
    m_persistence = std::make_unique<PersistenceManager>(db);
    wxPersistenceManager::Set(*m_persistence);

    auto mainFrame = new UI::MainFrame(env, db, cfg, m_options);

    std::for_each(
        m_plugins.begin(),
        m_plugins.end(),
        [mainFrame](auto plugin) { plugin->EmitEvent(libniix_event_mainwnd_created, mainFrame); });

    auto windowState = static_cast<pt::Core::Configuration::WindowState>(cfg->Get<int>("start_position").value());

    switch (windowState)
    {
    case pt::Core::Configuration::WindowState::Hidden:
        // Only valid if we have a notify icon
        if (cfg->Get<bool>("show_in_notification_area").value())
        {
#ifdef __WXMSW__
            mainFrame->MSWGetTaskBarButton()->Hide();
#endif
        }
        else
        {
            mainFrame->Show(true);
        }

        break;

    case pt::Core::Configuration::WindowState::Maximized:
        mainFrame->Show(true);
        mainFrame->Maximize();
        break;

    case pt::Core::Configuration::WindowState::Minimized:
        mainFrame->Iconize();
        mainFrame->Show(true);
        break;

    case pt::Core::Configuration::WindowState::Normal:
        mainFrame->Show(true);
        break;
    }

    mainFrame->HandleParams(m_options);

    return true;
}

int Application::OnRun()
{
    if (m_handedOver)
    {
        return 0;
    }

    return wxApp::OnRun();
}

void Application::OnInitCmdLine(wxCmdLineParser& parser)
{
    static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
        { wxCMD_LINE_OPTION, NULL, "wait-for-pid",  NULL,   wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, NULL, "silent",        NULL,   wxCMD_LINE_VAL_NONE ,  wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, NULL, "save-path",     NULL,   wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_PARAM,  NULL, NULL,           "params",wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
        { wxCMD_LINE_NONE }
    };

    parser.SetDesc(cmdLineDesc);
    parser.SetSwitchChars("-");
}

void Application::ActivateOtherInstance()
{
    json j;
    j["files"] = m_options.files;
    j["magnet_links"] = m_options.magnets;
    j["silent"] = m_options.silent;
    j["save_path"] = m_options.save_path;

    // The previous instance holds the single-instance lock, but its IPC server
    // may not be ready yet (or may be shutting down). wxWidgets would otherwise
    // show a raw DDE error dialog if the connection cannot be made, so suppress
    // its automatic logging and handle the outcome quietly ourselves.
    wxLogNull suppressConnectionErrors;

    wxClient client;
    wxConnectionBase* conn = nullptr;

    for (int attempt = 0; attempt < 20 && conn == nullptr; ++attempt)
    {
        conn = client.MakeConnection(
            "localhost",
            pt::IPC::Server::ServiceName(),
            "ApplicationOptions");

        if (conn == nullptr) { wxMilliSleep(50); }
    }

    if (conn)
    {
        conn->Execute(j.dump());
        conn->Disconnect();
    }
}

void Application::LoadPlugins(pt::Core::Environment* env, pt::Core::Configuration* cfg)
{
    // Plugins are shared libraries named Plugin_*.dll (Windows) or
    // Plugin_*.so (Linux), next to the executable or in the plugin directory.
    std::vector<fs::path> directories = { env->GetApplicationPath() };

#ifdef _WIN32
    const std::string extension = ".dll";
#else
    const std::string extension = ".so";
    directories.push_back(env->GetApplicationPath().parent_path() / "lib" / "niixtorrent" / "plugins");
    directories.push_back(fs::path(NIIX_PLUGIN_DIR));
#endif

    for (auto const& directory : directories)
    {
        std::error_code ec;

        for (auto const& p : fs::directory_iterator(directory, ec))
        {
            if (p.path().extension() != extension) { continue; }

            auto const filename = p.path().filename().string();
            if (filename.rfind("Plugin", 0) != 0) { continue; }

            BOOST_LOG_TRIVIAL(info) << "Loading plugin from " << p.path();

            auto plugin = API::IPlugin::Load(p.path(), env, cfg);

            if (plugin != nullptr)
            {
                m_plugins.push_back(plugin);
            }
        }
    }
}

void Application::WaitForPreviousInstance(long pid)
{
    // Used when restarting to apply a new language or theme.
#ifdef _WIN32
    HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (hProc == NULL) { return; }
    WaitForSingleObject(hProc, 10000);
    CloseHandle(hProc);
#else
    for (int i = 0; i < 100; i++)
    {
        if (kill(static_cast<pid_t>(pid), 0) != 0 && errno == ESRCH) { return; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
}
