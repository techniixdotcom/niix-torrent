#include "httpclient.hpp"

#include <wx/sstream.h>
#include <wx/webrequest.h>

#include <boost/log/trivial.hpp>

#include "../buildinfo.hpp"

using pt::Http::HttpClient;

HttpClient::HttpClient()
{
}

HttpClient::~HttpClient()
{
}

void HttpClient::Get(wxString const& url, std::function<void(int, std::string const&)> const& callback)
{
#if wxUSE_WEBREQUEST
    wxWebSession& session = wxWebSession::GetDefault();

    if (!session.IsOpened())
    {
        BOOST_LOG_TRIVIAL(warning) << "HTTP support is not available";
        callback(0, std::string());
        return;
    }

    wxWebRequest request = session.CreateRequest(this, url);

    if (!request.IsOk())
    {
        BOOST_LOG_TRIVIAL(warning) << "Could not create HTTP request for " << url.utf8_string();
        callback(0, std::string());
        return;
    }

    request.SetHeader("User-Agent", wxString("NiiXTorrent/") + pt::BuildInfo::version());

    int requestId = request.GetId();

    this->Bind(
        wxEVT_WEBREQUEST_STATE,
        [callback, requestId](wxWebRequestEvent& evt)
        {
            if (evt.GetId() != requestId) { evt.Skip(); return; }

            switch (evt.GetState())
            {
            case wxWebRequest::State_Completed:
            {
                std::string body = evt.GetResponse().AsString().utf8_string();
                callback(evt.GetResponse().GetStatus(), body);
                break;
            }

            case wxWebRequest::State_Failed:
            case wxWebRequest::State_Cancelled:
            case wxWebRequest::State_Unauthorized:
                BOOST_LOG_TRIVIAL(warning) << "HTTP request failed: " << evt.GetErrorDescription().utf8_string();
                callback(evt.GetResponse().IsOk() ? evt.GetResponse().GetStatus() : 0, std::string());
                break;

            default:
                break;
            }
        });

    request.Start();
#else
    (void)url;
    callback(0, std::string());
#endif
}
