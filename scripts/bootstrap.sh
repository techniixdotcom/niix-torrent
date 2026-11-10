#!/usr/bin/env bash
# Fetches and builds the pinned third-party libraries for a native Linux build
# of NiiX Torrent into vendor/linux (nothing is installed system-wide).
#
#   scripts/bootstrap.sh            build what is missing
#   scripts/bootstrap.sh --force    rebuild everything
#
# The same versions are used by the Flatpak manifest
# (packaging/flatpak/io.github.niixtorrent.NiiXTorrent.yml) and, on Windows,
# by scripts/bootstrap.ps1 + vcpkg.json. Keep them in sync when updating.
#
# Missing system packages (compiler, CMake, Ninja, GTK 3, ...) are installed
# automatically - see scripts/prereqs.sh.
set -euo pipefail

BOOST_VERSION=1.92.0
FMT_VERSION=12.2.1
JSON_VERSION=3.12.0
LIBTORRENT_VERSION=2.1.2
WXWIDGETS_VERSION=3.3.4
ANTLR_VERSION=4.13.2

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENDOR="$ROOT/vendor"
PREFIX="$VENDOR/linux"
SOURCES="$VENDOR/linux-src"
JOBS="${JOBS:-$(nproc)}"
FORCE=0
[[ "${1:-}" == "--force" ]] && FORCE=1

# shellcheck source=scripts/prereqs.sh
source "$ROOT/scripts/prereqs.sh"
ensure_prereqs native

mkdir -p "$PREFIX" "$SOURCES"

# done <name> <version>: true when that version is already built.
done_marker() { echo "$PREFIX/.built-$1-$2"; }
is_done()     { [[ $FORCE -eq 0 && -f "$(done_marker "$1" "$2")" ]]; }
mark_done()   { rm -f "$PREFIX"/.built-"$1"-*; touch "$(done_marker "$1" "$2")"; }

# fetch <url> <file>
fetch() {
    [[ -s "$SOURCES/$2" ]] && return 0
    echo "[download] $2"
    curl -fsSL --retry 3 -o "$SOURCES/$2.part" "$1"
    mv "$SOURCES/$2.part" "$SOURCES/$2"
}

# unpack <archive> <directory>
unpack() {
    rm -rf "${SOURCES:?}/$2"
    mkdir -p "$SOURCES/$2"
    tar -xf "$SOURCES/$1" -C "$SOURCES/$2" --strip-components=1
}

cmake_install() {
    local src="$1"; shift
    cmake -S "$src" -B "$src/_build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        "$@" >/dev/null
    cmake --build "$src/_build" -j "$JOBS"
    cmake --install "$src/_build" >/dev/null
}

# --- ANTLR C++ runtime (compiled as part of the application) -----------------
if [[ $FORCE -eq 1 || ! -f "$VENDOR/antlr4/runtime/Cpp/runtime/src/antlr4-runtime.h" ]]; then
    echo "[clone] antlr4 $ANTLR_VERSION"
    rm -rf "$VENDOR/antlr4"
    git clone --quiet --depth 1 --branch "$ANTLR_VERSION" https://github.com/antlr/antlr4 "$VENDOR/antlr4"
fi

# --- Boost (headers + Boost.Log and what it needs) ---------------------------
if ! is_done boost "$BOOST_VERSION"; then
    fetch "https://github.com/boostorg/boost/releases/download/boost-$BOOST_VERSION/boost-$BOOST_VERSION-b2-nodocs.tar.xz" "boost-$BOOST_VERSION.tar.xz"
    unpack "boost-$BOOST_VERSION.tar.xz" boost
    echo "[build] boost $BOOST_VERSION"
    (
        cd "$SOURCES/boost"
        ./bootstrap.sh --prefix="$PREFIX" \
            --with-libraries=log,filesystem,thread,system,date_time,atomic,chrono,regex,random >/dev/null
        ./b2 -j"$JOBS" -d0 variant=release link=static cxxflags=-fPIC cflags=-fPIC install >/dev/null
    )
    mark_done boost "$BOOST_VERSION"
fi

# --- {fmt} --------------------------------------------------------------------
if ! is_done fmt "$FMT_VERSION"; then
    echo "[build] fmt $FMT_VERSION"
    rm -rf "$SOURCES/fmt"
    git clone --quiet --depth 1 --branch "$FMT_VERSION" https://github.com/fmtlib/fmt "$SOURCES/fmt"
    cmake_install "$SOURCES/fmt" -DFMT_TEST=OFF -DFMT_DOC=OFF
    mark_done fmt "$FMT_VERSION"
fi

# --- nlohmann/json ------------------------------------------------------------
if ! is_done json "$JSON_VERSION"; then
    fetch "https://github.com/nlohmann/json/releases/download/v$JSON_VERSION/json.tar.xz" "json-$JSON_VERSION.tar.xz"
    unpack "json-$JSON_VERSION.tar.xz" json
    echo "[build] nlohmann-json $JSON_VERSION"
    cmake_install "$SOURCES/json" -DJSON_BuildTests=OFF
    mark_done json "$JSON_VERSION"
fi

# --- libtorrent ---------------------------------------------------------------
if ! is_done libtorrent "$LIBTORRENT_VERSION"; then
    fetch "https://github.com/arvidn/libtorrent/releases/download/v$LIBTORRENT_VERSION/libtorrent-rasterbar-$LIBTORRENT_VERSION.tar.gz" "libtorrent-$LIBTORRENT_VERSION.tar.gz"
    unpack "libtorrent-$LIBTORRENT_VERSION.tar.gz" libtorrent
    echo "[build] libtorrent $LIBTORRENT_VERSION (this takes a while)"
    cmake_install "$SOURCES/libtorrent" -DCMAKE_CXX_STANDARD=20 -DBUILD_SHARED_LIBS=OFF -Ddeprecated-functions=OFF
    mark_done libtorrent "$LIBTORRENT_VERSION"
fi

# --- wxWidgets ----------------------------------------------------------------
# The "-tray" suffix forces a rebuild of trees built before wxWidgets was
# configured with AppIndicator support.
if ! is_done wxwidgets "$WXWIDGETS_VERSION-tray"; then
    fetch "https://github.com/wxWidgets/wxWidgets/releases/download/v$WXWIDGETS_VERSION/wxWidgets-$WXWIDGETS_VERSION.tar.bz2" "wxWidgets-$WXWIDGETS_VERSION.tar.bz2"
    unpack "wxWidgets-$WXWIDGETS_VERSION.tar.bz2" wxwidgets
    echo "[build] wxWidgets $WXWIDGETS_VERSION (this takes a while)"
    cmake_install "$SOURCES/wxwidgets" \
        -DwxBUILD_SHARED=OFF -DwxBUILD_TOOLKIT=gtk3 \
        -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF -DwxBUILD_DEMOS=OFF \
        -DwxUSE_WEBVIEW=OFF -DwxUSE_MEDIACTRL=OFF -DwxUSE_OPENGL=OFF \
        -DwxUSE_STC=OFF -DwxUSE_RICHTEXT=OFF -DwxUSE_RIBBON=OFF \
        -DwxUSE_APPINDICATOR=ON
    grep -q "^wxUSE_APPINDICATOR:.*=ON" "$SOURCES/wxwidgets/_build/CMakeCache.txt" || {
        echo "error: wxWidgets was built without AppIndicator support - the tray icon would not work." >&2
        echo "       Install the libayatana-appindicator3 development package and run again." >&2
        exit 1
    }
    mark_done wxwidgets "$WXWIDGETS_VERSION-tray"
fi

echo
echo "All dependencies are ready in $PREFIX. Next:  ./build.sh native"
