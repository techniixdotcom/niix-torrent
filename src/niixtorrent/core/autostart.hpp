#pragma once

namespace pt
{
namespace Core
{
    // Launch NiiX Torrent when the user signs in.
    //
    //   Windows : HKCU\Software\Microsoft\Windows\CurrentVersion\Run
    //   Linux   : ~/.config/autostart/<app-id>.desktop, or the Background
    //             portal when running inside a Flatpak sandbox.
    class AutoStart
    {
    public:
        static bool IsEnabled();
        static void SetEnabled(bool enabled);
    };
}
}
