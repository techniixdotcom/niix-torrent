Building from source
====================

The version number lives in the ``VERSION`` file at the top of the source tree
and is picked up by CMake, the Windows installer and the build scripts.


Windows
-------

Requirements: Windows 10 or 11 (x64) and an internet connection. ``build.ps1``
downloads and installs everything else that is missing: Git, CMake, the Visual
Studio C++ build tools and - for the installer - Inno Setup 7. It uses
``winget`` where available and otherwise downloads the installers directly
from the vendors; Windows asks for administrator approval when an installer
needs it. Pass ``-SkipInstall`` to only check for the tools.

A first build needs about 15 GB of free disk space on the drive holding the
project (the libraries are compiled from source), plus about 8 GB on ``C:`` if
the Visual Studio build tools still have to be installed. The script checks
this before it starts.

Double-click ``build.cmd`` (or run ``.\build.ps1`` in PowerShell) and choose::

    1) Build the installer   -> dist\NiiXTorrent-<version>-x64-setup.exe
    2) Upload the source code to GitHub

To skip the question, for scripts and CI:

.. code-block:: powershell

    .\build.ps1 -Target Installer
    .\build.ps1 -Target Upload
    .\build.ps1 -Target Clean
    .\build.ps1 -Target Installer -Platform x86     # 32-bit build
    .\build.ps1 -Target Installer -WithCrashpad     # include the optional crash reporter

What happens:

1. ``scripts\bootstrap.ps1`` clones wxWidgets, vcpkg and the ANTLR runtime
   into ``vendor\`` at pinned versions.
2. CMake configures the project; vcpkg builds libtorrent, Boost, fmt,
   nlohmann/json, OpenSSL and SQLite as pinned by ``vcpkg.json``.
3. ``cmake --install`` stages the application files (including the Visual C++
   runtime DLLs) in ``dist\<platform>``.
4. Inno Setup compiles ``packaging\windows\NiiXTorrent.iss`` into the setup
   program.


Linux
-----

Flatpak
~~~~~~~

.. code-block:: bash

    ./build.sh

and choose::

    1) Build the Flatpak   -> dist/NiiXTorrent-<version>.flatpak
    2) Upload the source code to GitHub

``./build.sh build`` and ``./build.sh upload`` skip the question.
``flatpak`` and ``flatpak-builder`` are installed automatically if missing.

Building compiles every dependency inside the Freedesktop SDK, installs the
result for the current user and writes ``dist/NiiXTorrent-<version>.flatpak``,
a single file that can be installed on other machines. The manifest is
``packaging/flatpak/io.github.niixtorrent.NiiXTorrent.yml``.

Native build (for development)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The script installs what is missing with the distribution's package manager
(``apt``, ``dnf``, ``pacman`` or ``zypper``, asking for ``sudo`` when needed):
a C++ compiler, CMake, Ninja, Git, curl, pkg-config and the development files
for GTK 3, OpenSSL, SQLite, libcurl and libnotify. If the distribution's CMake
is older than 3.28, a current CMake is unpacked into ``vendor/tools`` and used
from there. Set ``NIIX_NO_INSTALL=1`` to only check and report what is missing.

.. code-block:: bash

    ./build.sh native      # build-linux/niixtorrent

``scripts/bootstrap.sh`` builds the pinned versions of Boost, fmt,
nlohmann/json, libtorrent and wxWidgets into ``vendor/linux`` first (once;
this takes a while), because distributions do not ship versions that are
recent enough yet. To use libraries that are already installed instead, call
CMake directly::

    cmake -S . -B build-linux -G Ninja -DCMAKE_PREFIX_PATH=/path/to/prefix
    cmake --build build-linux

libtorrent 2.1 or newer and wxWidgets 3.3 or newer are required.


Updating dependencies
---------------------

================== ========================================================
Dependency         Where the version is pinned
================== ========================================================
wxWidgets, ANTLR   ``scripts/bootstrap.ps1``, ``scripts/bootstrap.sh``,
                   Flatpak manifest
vcpkg libraries    ``builtin-baseline`` in ``vcpkg.json`` (a vcpkg commit)
                   and the vcpkg tag in ``scripts/bootstrap.ps1``
Linux libraries    ``scripts/bootstrap.sh`` and the Flatpak manifest
Flatpak runtime    ``runtime-version`` in the Flatpak manifest
================== ========================================================

After changing a version on Windows run ``scripts\bootstrap.ps1 -Force``.


Uploading the source
--------------------

Option 2 of the build scripts commits the current tree and pushes it to
https://github.com/techniixdotcom/niix-torrent (it asks for a short
description of the changes). Build output and downloaded dependencies are left
out, existing history is kept and nothing is force-pushed. Releases are
created by hand on GitHub from the files in ``dist``.
