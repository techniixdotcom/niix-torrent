# NiiX Torrent

**Version 2.0 — maintained by: cuteLiLi / techniix / QuacK / decoi**

A fast, lightweight BitTorrent client for Windows and Linux, built in modern
C++ on top of Rasterbar-libtorrent.

NiiX Torrent is a fork of [PicoTorrent](https://github.com/picotorrent/picotorrent),
which is no longer being developed. I will continue and maintain it under the new
name **NiiX Torrent**, rebuilt against the latest versions of every dependency and
carrying the NiiX brand palette throughout the UI.

- BitTorrent v2 (BEP-52), v1, and hybrid torrents
- DHT, PeX, LSD, UPnP
- Dark and light themes with the NiiX accent colours (follows the system by default)
- Windows installer and uninstaller, and a Flatpak for Linux
- Peer-ID prefix `-NX-`, user agent `NiiXTorrent/x.y.z`

## Install

- **Windows 10 / 11:** run `NiiXTorrent-<version>-x64-setup.exe`
- **Linux:** `flatpak install NiiXTorrent-<version>.flatpak`

## Build

Run the build script and pick what you want - it downloads and installs
everything it needs:

- **Windows:** double-click `build.cmd`
- **Linux:** `./build.sh`

```
  1) Build the installer / Flatpak   -> dist/
  2) Upload the source code to GitHub
```

Built with libtorrent 2.1.2, wxWidgets 3.3.4, Boost 1.92, fmt 12.2,
nlohmann/json 3.12, OpenSSL 3.6, SQLite and ANTLR 4.13.2.



translations made with love by:

🇮🇩 Indonesian by @zmni
🇮🇹 Italian by @bovirus
🇹🇷 Turkish by @xzork, @hipofiz
🇸🇦 Arabic (Saudi Arabia) by @gitntel
🇺🇦 Ukranian by @molny, @mxve
🇬🇪 Georgian by @arabuli
🇻🇳 Vietnamese by @Doraemon-kun
🇵🇱 Polish by @luki446
🇷🇸 Serbian by @laikexpert, @Artyom2034
🇯🇵 Japanese by @xE0F9
🇬🇧 English by @comradekingu
🇳🇴 Norwegian (Bokmål) by @comradekingu
🇧🇬 Bulgarian by @StoyanDimitrov
🇫🇷 French by @TechnoGreg
🇷🇺 Russian by @AHOHNMYC
🇱🇰 Sinhalese by @Aikatsui
🇪🇸 Spanish by @DerianAndre
🇧🇷 Portuguese (Brazil) by @nero-0, @mmagian
🇩🇪 German by @mxve
🇭🇺 Hungarian by @mxve
🇱🇹 Lithuanian by @mxve
🇱🇻 Latvian by @mxve
🇸🇪 Swedish by @mxve
🇨🇿 Czech by @DavidJares
🇳🇱 Dutch by @Stephan-P
🇸🇰 Slovak by @xzpyth
🇨🇳 Chinese (Simplified) by @mayli, @KaMingChung
🇭🇷 Croatian by @migwynkriid
