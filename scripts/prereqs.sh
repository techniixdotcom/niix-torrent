#!/usr/bin/env bash
# Installs whatever is needed to build NiiX Torrent on Linux. Sourced by
# build.sh and scripts/bootstrap.sh:
#
#   source scripts/prereqs.sh
#   ensure_prereqs native     # compiler, CMake, Ninja, GTK 3 and friends
#   ensure_prereqs flatpak    # flatpak + flatpak-builder
#
# System packages are installed with the distribution's package manager
# (apt, dnf, pacman or zypper) - through sudo when not running as root. A
# CMake that is too old is replaced by an official binary unpacked into
# vendor/tools (nothing system-wide is touched for that).
#
# Set NIIX_NO_INSTALL=1 to only check and report what is missing.

NIIX_CMAKE_MIN="3.28"
NIIX_CMAKE_VERSION="4.4.5"      # downloaded when the system CMake is too old

_prereq_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

_have() { command -v "$1" >/dev/null 2>&1; }

_version_ge() {  # _version_ge <have> <want>
    [ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]
}

_cmake_ok() {
    _have cmake || return 1
    local v
    v="$(cmake --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)"
    [ -n "$v" ] && _version_ge "$v" "$NIIX_CMAKE_MIN"
}

_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif _have sudo; then
        sudo "$@"
    else
        echo "error: administrator rights are needed to install packages, but 'sudo' is not available." >&2
        echo "       Run as root:  $*" >&2
        return 1
    fi
}

_package_manager() {
    for pm in apt-get dnf pacman zypper; do
        if _have "$pm"; then echo "$pm"; return 0; fi
    done
    return 1
}

# _install_packages <what> : installs the package set for the detected distribution
_install_packages() {
    local what="$1" pm
    pm="$(_package_manager)" || {
        echo "error: no supported package manager found (apt, dnf, pacman, zypper)." >&2
        echo "       Install the build requirements listed in docs/src/building.rst by hand." >&2
        return 1
    }

    local pkgs=()
    case "$pm:$what" in
        apt-get:native) pkgs=(build-essential cmake ninja-build git curl ca-certificates pkg-config tar xz-utils bzip2
                              libgtk-3-dev libssl-dev libsqlite3-dev libcurl4-openssl-dev libnotify-dev
                              libayatana-appindicator3-dev) ;;
        dnf:native)     pkgs=(gcc-c++ make cmake ninja-build git curl ca-certificates pkgconf-pkg-config tar xz bzip2
                              gtk3-devel openssl-devel sqlite-devel libcurl-devel libnotify-devel
                              libayatana-appindicator-gtk3-devel) ;;
        pacman:native)  pkgs=(base-devel cmake ninja git curl ca-certificates pkgconf tar xz bzip2
                              gtk3 openssl sqlite libnotify libayatana-appindicator) ;;
        zypper:native)  pkgs=(gcc-c++ make cmake ninja git curl ca-certificates pkgconf-pkg-config tar xz bzip2
                              gtk3-devel libopenssl-devel sqlite3-devel libcurl-devel libnotify-devel
                              libayatana-appindicator3-devel) ;;
        *:flatpak)      pkgs=(flatpak flatpak-builder) ;;
    esac

    echo "[install] ${pkgs[*]}"
    case "$pm" in
        apt-get)
            _as_root env DEBIAN_FRONTEND=noninteractive apt-get update -qq || true
            _as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "${pkgs[@]}"
            ;;
        dnf)    _as_root dnf install -y "${pkgs[@]}" ;;
        pacman) _as_root pacman -Sy --needed --noconfirm "${pkgs[@]}" ;;
        zypper) _as_root zypper --non-interactive install --no-recommends "${pkgs[@]}" ;;
    esac
}

# Official CMake binary into vendor/tools when the distribution's is too old.
_install_local_cmake() {
    local arch dir url
    case "$(uname -m)" in
        x86_64)        arch=x86_64 ;;
        aarch64|arm64) arch=aarch64 ;;
        *) echo "error: CMake $NIIX_CMAKE_MIN or newer is required and no binary is available for $(uname -m)." >&2; return 1 ;;
    esac

    dir="$_prereq_root/vendor/tools/cmake-$NIIX_CMAKE_VERSION"
    if [ ! -x "$dir/bin/cmake" ]; then
        url="https://github.com/Kitware/CMake/releases/download/v$NIIX_CMAKE_VERSION/cmake-$NIIX_CMAKE_VERSION-linux-$arch.tar.gz"
        echo "[download] CMake $NIIX_CMAKE_VERSION (the installed one is older than $NIIX_CMAKE_MIN)"
        mkdir -p "$dir"
        curl -fsSL --retry 3 "$url" | tar -xz -C "$dir" --strip-components=1
    fi
    export PATH="$dir/bin:$PATH"
}

_missing_native() {
    local missing=()
    for tool in c++ cc git curl ninja pkg-config tar xz bzip2; do
        _have "$tool" || missing+=("$tool")
    done
    _have cmake || missing+=("cmake")
    if _have pkg-config; then
        # ayatana-appindicator3: the notification area icon on Wayland and on
        # current desktops (StatusNotifierItem); without it there is no tray icon.
        for module in gtk+-3.0 openssl sqlite3 libcurl libnotify ayatana-appindicator3-0.1; do
            pkg-config --exists "$module" 2>/dev/null || missing+=("$module")
        done
    fi
    echo "${missing[*]}"
}

_missing_flatpak() {
    local missing=()
    for tool in flatpak flatpak-builder; do
        _have "$tool" || missing+=("$tool")
    done
    echo "${missing[*]}"
}

ensure_prereqs() {
    local what="${1:-native}" missing
    missing="$(_missing_"$what")"

    if [ -n "$missing" ]; then
        echo "Missing build requirements: $missing"
        if [ "${NIIX_NO_INSTALL:-0}" = "1" ]; then
            echo "error: NIIX_NO_INSTALL=1 is set - install them and run again." >&2
            return 1
        fi

        _install_packages "$what"

        missing="$(_missing_"$what")"
        if [ -n "$missing" ]; then
            echo "error: still missing after installing packages: $missing" >&2
            return 1
        fi
    fi

    if [ "$what" = "native" ] && ! _cmake_ok; then
        if [ "${NIIX_NO_INSTALL:-0}" = "1" ]; then
            echo "error: CMake $NIIX_CMAKE_MIN or newer is required." >&2
            return 1
        fi
        _install_local_cmake
        _cmake_ok || { echo "error: could not set up CMake $NIIX_CMAKE_VERSION." >&2; return 1; }
    fi

    echo "[ok] build requirements for the $what build are installed"
}
