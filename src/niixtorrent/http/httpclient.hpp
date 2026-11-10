#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <functional>
#include <string>

namespace pt
{
namespace Http
{
    // Minimal asynchronous HTTP GET built on wxWebRequest, which uses WinHTTP
    // on Windows and libcurl on Linux. The callback runs on the UI thread.
    class HttpClient : public wxEvtHandler
    {
    public:
        HttpClient();
        virtual ~HttpClient();

        void Get(wxString const& url, std::function<void(int, std::string const&)> const& callback);
    };
}
}
