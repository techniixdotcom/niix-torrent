#!/usr/bin/env bash
# Builds the NiiX Torrent Flatpak on Linux, or uploads the source code.
#
#   ./build.sh            asks:  1) build the Flatpak   2) upload the source code
#
# To skip the question:
#   ./build.sh build      build the Flatpak, install it for the current user and
#                         write dist/NiiXTorrent-<version>.flatpak
#   ./build.sh upload     upload the source code to GitHub
#   ./build.sh clean      remove build folders and dist
#
# Everything needed is fetched automatically: flatpak and flatpak-builder are
# installed with apt, dnf, pacman or zypper (asking for sudo when required)
# and the runtime comes from Flathub.
# Set NIIX_NO_INSTALL=1 to only check for missing requirements.
#
# For development there is also "./build.sh native", which builds a plain
# executable (build-linux/niixtorrent) instead of a package.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"
APP_ID="io.github.niixtorrent.NiiXTorrent"
BUILD_DIR="$ROOT/build-linux"
TARGET="${1:-}"

if [[ -z "$TARGET" ]]; then
    echo
    echo "NiiX Torrent $VERSION"
    echo "  1) Build the Flatpak  (dist/NiiXTorrent-$VERSION.flatpak)"
    echo "  2) Upload the source code to GitHub"
    echo
    choice=""
    while [[ "$choice" != "1" && "$choice" != "2" ]]; do
        read -r -p "Choose 1 or 2: " choice || exit 1
    done
    [[ "$choice" == "1" ]] && TARGET="build" || TARGET="upload"
fi

# shellcheck source=scripts/prereqs.sh
source "$ROOT/scripts/prereqs.sh"

native_build() {
    # Installs compiler, CMake, Ninja and GTK if missing, then builds the
    # pinned libraries.
    ensure_prereqs native
    "$ROOT/scripts/bootstrap.sh"

    cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}" \
        -DCMAKE_PREFIX_PATH="$ROOT/vendor/linux" \
        -DCMAKE_INSTALL_PREFIX="${PREFIX:-/usr/local}"
    cmake --build "$BUILD_DIR" -j "${JOBS:-$(nproc)}"

    echo
    echo "Built $BUILD_DIR/niixtorrent"
}

case "$TARGET" in
    native)
        native_build
        echo "This is a bare executable for development. For the installable package run:  ./build.sh build"
        ;;

    upload)
        read -r -p "Describe the changes (press Enter for 'NiiX Torrent $VERSION'): " message || message=""
        if [[ -n "${message// /}" ]]; then
            exec "$ROOT/scripts/publish.sh" "$message"
        else
            exec "$ROOT/scripts/publish.sh"
        fi
        ;;

    build|flatpak)
        ensure_prereqs flatpak

        flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

        flatpak-builder --user --install --force-clean \
            --install-deps-from=flathub \
            --repo="$ROOT/build-flatpak/repo" \
            --state-dir="$ROOT/build-flatpak/state" \
            "$ROOT/build-flatpak/app" \
            "$ROOT/packaging/flatpak/$APP_ID.yml"

        mkdir -p "$ROOT/dist"
        flatpak build-bundle "$ROOT/build-flatpak/repo" \
            "$ROOT/dist/NiiXTorrent-$VERSION.flatpak" "$APP_ID" \
            --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo

        echo
        echo "Installed. Start it with:   flatpak run $APP_ID"
        echo "Bundle for other machines:  dist/NiiXTorrent-$VERSION.flatpak"
        echo "Uninstall with:             flatpak uninstall $APP_ID   (add --delete-data to remove settings too)"
        ;;

    clean)
        rm -rf "$BUILD_DIR" "$ROOT/build-flatpak" "$ROOT/dist" "$ROOT/.flatpak-builder"
        ;;

    *)
        sed -n '2,18p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
        exit 1
        ;;
esac
