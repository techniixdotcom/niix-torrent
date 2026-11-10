Module definitions taken from Flathub's shared modules
(https://github.com/flathub/shared-modules, commit e69357f, 2026-11-10).

Local change: libayatana-appindicator is updated from 0.5.94 to 0.6.1. That
release brings its indicator code along, so the ayatana-ido and
libayatana-indicator modules are no longer needed, and it adds the "activate"
signal that lets a left click on the tray icon restore the window.
