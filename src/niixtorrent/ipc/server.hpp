#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/ipc.h>

namespace pt
{
namespace UI
{
    class MainFrame;
}
namespace IPC
{
    class Server : public wxServer
    {
    public:
        Server(UI::MainFrame* mainFrame);
        virtual ~Server();

        // DDE service name on Windows, a Unix domain socket in the user's
        // runtime directory elsewhere.
        static wxString ServiceName();

        // Directory for per-session runtime files (socket, instance lock).
        static wxString RuntimeDirectory();

        virtual wxConnectionBase* OnAcceptConnection(const wxString& topic) wxOVERRIDE;

    private:
        UI::MainFrame* m_frame;
    };
}
}
