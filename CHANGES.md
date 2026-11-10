# NiiX Torrent — Revival & Rebrand Notes

This is a modernized, rebranded continuation of the discontinued PicoTorrent
project.

## 2.0.0

### Linux version

- The application now builds and runs on Linux (wxGTK). All Win32-only code has
  a portable counterpart: paths follow the XDG directories, single-instance
  hand-over uses a Unix socket, "start at login" writes an XDG autostart entry
  (or asks the desktop portal inside Flatpak), "Show in folder" talks to the
  file manager over D-Bus, notifications use the desktop notification service.
- Packaged as a Flatpak (`packaging/flatpak`), with desktop entry, AppStream
  metadata, icons, and `.torrent` / `magnet:` handlers (`packaging/linux`).
- `./build.sh` builds the Flatpak (`./build.sh native` gives a plain
  development executable).
- The update-check plugin stays Windows-only; Flatpak delivers updates.

### Windows installer

- New Inno Setup 7 installer and uninstaller (`packaging/windows/NiiXTorrent.iss`)
  replacing the WiX MSI and its .NET bootstrapper: all-users or per-user
  install, optional desktop icon, `.torrent` and magnet associations,
  firewall rule, start with Windows, close-running-app detection, silent
  mode, upgrade in place, removal of the previous MSI installation.
- The uninstaller removes program files, shortcuts, associations, firewall
  rule and start-up entry, and asks before deleting settings and the torrent
  list (`/REMOVEDATA` / `/KEEPDATA` for unattended use).
- The Visual C++ runtime is shipped app-local; no redistributable download.
- `build.cmd` / `build.ps1` builds the installer. Cake, GitVersion and the
  .NET SDK are no longer needed.

### Interface

- Themes: **System** (default), **Dark** and **Light** in Preferences > General.
  On Windows the dark theme recolours the native controls with the NiiX
  palette; on Linux the GTK theme is tinted to match.
- New flat progress bars with percentage, coloured by state (downloading,
  seeding, checking, paused, error); status text uses the same colours.
- Redrawn piece map (sharp at any size and DPI), calmer zebra striping,
  roomier rows, sensible default window size and column widths.
- Filter bar with icon, placeholder example and Esc-to-clear.
- Reworked About dialog; tidier Add Torrent dialog; shortcuts shown in menus;
  check marks on the View menu toggles.
- Drag and drop `.torrent` files onto the window.
- Per-monitor (v2) DPI awareness through the application manifest; icons are
  embedded and rendered per DPI scale.
- The window can no longer be hidden to a notification area that does not exist.

### Dependencies

| | from | to |
|---|---|---|
| libtorrent | 2.0.x | 2.1.2 |
| wxWidgets | 3.3.2 | 3.3.4 |
| Boost | (vcpkg master) | 1.92.0 |
| fmt | (vcpkg master) | 12.2 |
| nlohmann/json | (vcpkg master) | 3.12.0 |
| vcpkg | unpinned `master` | release 2026.10.16, pinned by `builtin-baseline` |
| Crashpad | picotorrent forks | getsentry/crashpad (optional, `-WithCrashpad`) |

- The code was migrated to the libtorrent 2.1 API (the deprecated 1.x/2.0
  functions are no longer compiled in by default): `load_torrent_buffer`,
  `list_files`/`create_file_entry`, `torrent_info::layout`,
  `peer_info::remote_endpoint`, `write_torrent_file` for exports, trackers and
  comments carried by `add_torrent_params`.
- HTTP requests use `wxWebRequest` (WinHTTP on Windows, libcurl on Linux)
  instead of a hand-written WinHTTP client.
- SQL migrations and artwork are compiled into the executable on every
  platform (`cmake/EmbedResources.cmake`) instead of Windows resources.
- C++20, CMake 3.28+.

### Behaviour changes worth knowing

- Crash reports are no longer uploaded anywhere unless a collector URL is
  configured at build time (`-DNIIX_CRASHPAD_URL=...`); previously the
  upstream project's Sentry endpoint was still compiled in.
- Text entered in dialogs (paths, tracker URLs, labels) is converted as UTF-8
  everywhere, which fixes non-ASCII paths when creating torrents.
- Source files that were missing from the tree (torrent list, details views,
  translator, piece map) were restored from PicoTorrent, and the source
  directory was renamed to `src/niixtorrent` to match the build files.

## Rebrand (PicoTorrent -> NiiX Torrent)

- Product name, window titles, message boxes, About dialog, taskbar tooltip.
- Version resource (`version.h`): product name, description, original filename
  (`NiiXTorrent.exe`), copyright.
- BitTorrent identity: user agent `NiiXTorrent/x.y.z`; peer-ID fingerprint
  prefix changed from `-PI-` to `-NX-`.
- HTTP/GeoIP user agents, crash-report release tag, IPC service name, autorun
  registry value, AppData folder, SQLite DB and log filenames, `HKCU\Software`
  key.
- MSI installer (WiX): product/feature/shortcut names, file associations and
  ProgIds, firewall rule, registry capabilities — all reissued with **fresh
  GUIDs** so NiiX Torrent installs as a distinct product.
- All 37 language files and the documentation.

## Dark theme only (superseded in 2.0.0 - see above)

- The theme selector has been removed from Preferences.
- `Configuration::IsDarkMode()` always returns `true`.
- Native Windows dark mode is forced at startup via
  `MSWEnableDarkMode(DarkMode_Always)`.
- Default `theme_id` migration value set to `"dark"`.

## Dependency / toolchain modernization

- `vcpkg.json`: schema added, OpenSSL made explicit; versions resolve from the
  `vendor/vcpkg` checkout (pull it to get the newest packages).
- wxWidgets pinned to the 3.3 stable line (first release with native dark mode).
- CMake minimum raised to 3.25; ANTLR4 runtime and utfcpp bumped; deprecated
  `git://` protocol replaced with `https://`.
- CI workflow updated to `actions/checkout@v4` and `actions/upload-artifact@v4`.
- Cake tool bumped to 4.0.0.

## Reliability decisions (documented on purpose)

- **C++ standard kept at C++17** (the standard the codebase was validated
  against) rather than forcing C++20, to avoid introducing compile breaks.
- **`/WX` (warnings-as-errors) relaxed to `/W4`** so deprecation warnings from
  newer dependency versions do not fail the build. `/guard:cf` (control-flow
  guard) is retained.
- The optional .NET installer bootstrapper keeps its internal
  `NiiXTorrentBootstrapper` assembly/namespace for build stability; only its
  user-facing strings are rebranded.
- The original backend endpoints (update check, GeoIP database) pointed at the
  discontinued project's server. They now use NiiX placeholders and are inert
  until you host your own.

## Assets

Icon and image assets in `res/` (`app.ico`, `file.ico`, etc.) are retained from
the original project. Replace them with custom NiiX artwork when ready.
