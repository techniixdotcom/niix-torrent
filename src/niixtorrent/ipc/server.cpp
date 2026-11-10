#include "server.hpp"

#include <wx/filename.h>
#include <wx/utils.h>

#include <boost/log/trivial.hpp>

#include "applicationoptionsconnection.hpp"
#include "../ui/mainframe.hpp"

using pt::IPC::Server;

Server::Server(pt::UI::MainFrame* mainFrame)
    : m_frame(mainFrame)
{
    if (!Create(ServiceName()))
    {
        BOOST_LOG_TRIVIAL(warning) << "Could not start the IPC server - torrents opened from other programs will not be handed over";
    }
}

wxString Server::RuntimeDirectory()
{
#ifdef _WIN32
    return wxEmptyString;
#else
    wxString dir;

    if (!wxGetEnv("XDG_RUNTIME_DIR", &dir) || dir.empty() || !wxFileName::DirExists(dir))
    {
        dir = wxFileName::GetTempDir();
    }

    return dir;
#endif
}

wxString Server::ServiceName()
{
#ifdef _WIN32
    return "NiiXTorrent";
#else
    return RuntimeDirectory() + wxString::Format("/niixtorrent-%s.sock", wxGetUserId());
#endif
}

Server::~Server()
{
}

wxConnectionBase* Server::OnAcceptConnection(const wxString& topic)
{
    if (topic == "ApplicationOptions")
    {
        return new ApplicationOptionsConnection(m_frame);
    }

    return nullptr;
}
