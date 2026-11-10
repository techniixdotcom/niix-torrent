Installing
==========

You should only install NiiX Torrent from trusted sources. New releases are
published on GitHub.


Windows
-------

NiiX Torrent runs on Windows 10 (1809 or later) and Windows 11.

Run ``NiiXTorrent-<version>-x64-setup.exe`` and follow the wizard. On the first
page you choose between

* **Install for all users** - installs to ``C:\Program Files\NiiX Torrent``
  and needs administrator rights, or
* **Install for me only** - installs to your profile and needs none.

The *Select Additional Tasks* page lets you pick

* a desktop shortcut,
* opening ``.torrent`` files and magnet links with NiiX Torrent,
* a Windows Firewall rule so other peers can connect to you (all-users installs
  only), and
* starting NiiX Torrent when you sign in.

Everything NiiX Torrent needs is included; no separate runtime has to be
installed. Running a newer setup over an existing installation upgrades it and
keeps your settings. An installation made with the earlier MSI-based installer
is removed automatically.

Unattended installation
~~~~~~~~~~~~~~~~~~~~~~~

The setup program understands the standard Inno Setup switches::

    NiiXTorrent-2.0.0-x64-setup.exe /VERYSILENT /NORESTART /ALLUSERS
    NiiXTorrent-2.0.0-x64-setup.exe /SILENT /CURRENTUSER /TASKS="assoctorrent,assocmagnet"

Task names: ``desktopicon``, ``assoctorrent``, ``assocmagnet``, ``firewall``,
``autostart``.

Uninstalling
~~~~~~~~~~~~

Open *Settings → Apps → Installed apps*, find *NiiX Torrent* and choose
*Uninstall*. The program, its shortcuts, file associations, firewall rule and
start-up entry are removed. You are asked whether your settings, torrent list
and logs (``%LOCALAPPDATA%\NiiX Torrent``) should be removed as well -
downloaded files are never touched.

For unattended removal run ``unins000.exe /VERYSILENT`` from the installation
folder. Add ``/REMOVEDATA`` to delete the settings too; without it they are
kept.


Linux
-----

NiiX Torrent is distributed as a Flatpak, which runs on every current
distribution. With `Flatpak <https://flatpak.org/setup/>`_ set up::

    flatpak install NiiXTorrent-<version>.flatpak
    flatpak run io.github.niixtorrent.NiiXTorrent

It then also appears in your application menu and is offered for ``.torrent``
files and magnet links.

Settings and the torrent list live in
``~/.var/app/io.github.niixtorrent.NiiXTorrent``.

Uninstall with::

    flatpak uninstall io.github.niixtorrent.NiiXTorrent

Add ``--delete-data`` to remove settings and the torrent list too.

.. note::

   The notification area icon needs a desktop with a system tray. Where there
   is none (stock GNOME, for example) the related options have no effect and
   the window always stays reachable from the task bar or dock.
