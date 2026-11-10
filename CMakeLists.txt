cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

cmake_policy(SET CMP0092 NEW) # MSVC: don't add /W3 by default
cmake_policy(SET CMP0091 NEW) # MSVC: runtime library selected through CMAKE_MSVC_RUNTIME_LIBRARY

# --------------------------------------------------------------------------
# Dependency source
#
#   Windows : vcpkg manifest mode (vendor/vcpkg, pinned by vcpkg.json) plus
#             wxWidgets and ANTLR from vendor/ (see scripts/bootstrap.ps1).
#   Linux   : system / Flatpak-provided packages by default. Pass
#             -DNIIX_USE_VCPKG=ON to use vcpkg there as well.
# --------------------------------------------------------------------------
if(WIN32)
    set(_niix_vcpkg_default ON)
else()
    set(_niix_vcpkg_default OFF)
endif()
option(NIIX_USE_VCPKG "Resolve third-party libraries through vendor/vcpkg" ${_niix_vcpkg_default})

if(NIIX_USE_VCPKG AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(_niix_vcpkg_toolchain "${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake")
    if(NOT EXISTS "${_niix_vcpkg_toolchain}")
        message(FATAL_ERROR
            "vendor/vcpkg was not found. Run scripts/bootstrap.ps1 (Windows) or "
            "scripts/bootstrap.sh --vcpkg (Linux) first, or configure with -DNIIX_USE_VCPKG=OFF.")
    endif()
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg-overlays/triplets")
    set(CMAKE_TOOLCHAIN_FILE "${_niix_vcpkg_toolchain}" CACHE STRING "vcpkg toolchain file")
endif()

# Single source of truth for the version number (also read by the installer
# scripts and the Flatpak metadata).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" NIIX_VERSION LIMIT_COUNT 1)
string(STRIP "${NIIX_VERSION}" NIIX_VERSION)

project(NiiXTorrent VERSION ${NIIX_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
include(cmake/EmbedResources.cmake)

# --------------------------------------------------------------------------
# Build metadata
# --------------------------------------------------------------------------
set(NIIX_GIT_BRANCH "" CACHE STRING "Branch name baked into the build")
set(NIIX_GIT_SHA    "" CACHE STRING "Short commit hash baked into the build")

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    if(NOT NIIX_GIT_SHA)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE NIIX_GIT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    endif()
    if(NOT NIIX_GIT_BRANCH)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE NIIX_GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    endif()
endif()
if(NOT NIIX_GIT_SHA)
    set(NIIX_GIT_SHA "0000000")
endif()
if(NOT NIIX_GIT_BRANCH)
    set(NIIX_GIT_BRANCH "main")
endif()

# Crash reports are only uploaded when a collector URL is configured at build
# time (or through the NIIXTORRENT_CRASHPAD_URL environment variable).
set(NIIX_CRASHPAD_URL "" CACHE STRING "Crash report upload URL (empty disables uploads)")

# --------------------------------------------------------------------------
# wxWidgets: vendored checkout if present, otherwise an installed 3.3+
# --------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/wx/CMakeLists.txt")
    set(wxBUILD_SHARED        OFF   CACHE BOOL   "" FORCE)
    set(wxBUILD_COMPATIBILITY "3.2" CACHE STRING "" FORCE)
    set(wxUSE_EXCEPTIONS      OFF   CACHE BOOL   "" FORCE)
    set(wxUSE_WEBVIEW         OFF   CACHE BOOL   "" FORCE)
    set(wxUSE_MEDIACTRL       OFF   CACHE BOOL   "" FORCE)
    set(wxUSE_STC             OFF   CACHE BOOL   "" FORCE)
    set(wxUSE_RICHTEXT        OFF   CACHE BOOL   "" FORCE)
    set(wxUSE_RIBBON          OFF   CACHE BOOL   "" FORCE)
    set(wxUSE_OPENGL          OFF   CACHE BOOL   "" FORCE)
    add_subdirectory(vendor/wx EXCLUDE_FROM_ALL)
    set(NIIX_WX_LIBS wx::core wx::base wx::propgrid wx::net)
    message(STATUS "NiiX: using vendored wxWidgets (vendor/wx)")
else()
    if(NOT WIN32)
        # A static wxWidgets built with tray (AppIndicator) support refers to
        # this pkg-config target from its exported package; define it first.
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(APPINDICATOR QUIET IMPORTED_TARGET ayatana-appindicator3-0.1)
    endif()
    find_package(wxWidgets 3.3 CONFIG REQUIRED COMPONENTS core base propgrid net)
    set(NIIX_WX_LIBS wx::core wx::base wx::propgrid wx::net)
    message(STATUS "NiiX: using installed wxWidgets ${wxWidgets_VERSION}")
endif()

# --------------------------------------------------------------------------
# Crashpad (optional, Windows only)
# --------------------------------------------------------------------------
set(WITH_CRASHPAD OFF)
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/crashpad/CMakeLists.txt")
    set(CRASHPAD_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(vendor/crashpad EXCLUDE_FROM_ALL)
    set(WITH_CRASHPAD ON)
else()
    message(STATUS "NiiX: building WITHOUT crash reporting")
endif()

# --------------------------------------------------------------------------
# NiiX components
# --------------------------------------------------------------------------
add_subdirectory(src/pql)

# --------------------------------------------------------------------------
# Third-party packages
# --------------------------------------------------------------------------
find_package(Boost               CONFIG REQUIRED COMPONENTS log)
find_package(fmt                 CONFIG REQUIRED)
find_package(LibtorrentRasterbar CONFIG REQUIRED)
find_package(nlohmann_json       CONFIG REQUIRED)

find_package(unofficial-sqlite3 CONFIG QUIET)
if(TARGET unofficial::sqlite3::sqlite3)
    set(NIIX_SQLITE_LIB unofficial::sqlite3::sqlite3)
else()
    find_package(SQLite3 REQUIRED)
    # SQLite3::SQLite3 is the current name; older CMake only has SQLite::SQLite3.
    if(TARGET SQLite3::SQLite3)
        set(NIIX_SQLITE_LIB SQLite3::SQLite3)
    else()
        set(NIIX_SQLITE_LIB SQLite::SQLite3)
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/niixtorrent/buildinfo.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/buildinfo.cpp"
    @ONLY)

if(MSVC)
    # Keep PDBs for release builds so crash dumps can be symbolised.
    add_compile_options(/Zi /utf-8 /MP)
    add_link_options("$<$<CONFIG:Release>:/DEBUG;/INCREMENTAL:NO;/OPT:REF;/OPT:ICF>")
endif()

# --------------------------------------------------------------------------
# Translation database generator (host tool)
# --------------------------------------------------------------------------
file(GLOB NIIX_LANG_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lang/*.json")

add_executable(NiiXTorrent-coredb src/l10n/main.cpp)
target_link_libraries(NiiXTorrent-coredb PRIVATE nlohmann_json::nlohmann_json ${NIIX_SQLITE_LIB})

# --------------------------------------------------------------------------
# Application
# --------------------------------------------------------------------------
set(NIIX_SOURCES
    src/niixtorrent/application.cpp
    src/niixtorrent/crashpadinitializer.cpp
    src/niixtorrent/main.cpp
    src/niixtorrent/persistencemanager.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/generated/buildinfo.cpp"

    # API
    src/niixtorrent/api/libniix.cpp

    # BitTorrent
    src/niixtorrent/bittorrent/session.cpp
    src/niixtorrent/bittorrent/torrenthandle.cpp

    # Core
    src/niixtorrent/core/autostart.cpp
    src/niixtorrent/core/configuration.cpp
    src/niixtorrent/core/database.cpp
    src/niixtorrent/core/environment.cpp
    src/niixtorrent/core/utils.cpp

    # Http
    src/niixtorrent/http/httpclient.cpp

    # IPC
    src/niixtorrent/ipc/applicationoptionsconnection.cpp
    src/niixtorrent/ipc/server.cpp

    # Dialogs
    src/niixtorrent/ui/dialogs/aboutdialog.cpp
    src/niixtorrent/ui/dialogs/addmagnetlinkdialog.cpp
    src/niixtorrent/ui/dialogs/addtrackerdialog.cpp
    src/niixtorrent/ui/dialogs/addtorrentdialog.cpp
    src/niixtorrent/ui/dialogs/createtorrentdialog.cpp
    src/niixtorrent/ui/dialogs/listeninterfacedialog.cpp
    src/niixtorrent/ui/dialogs/preferencesadvancedpage.cpp
    src/niixtorrent/ui/dialogs/preferencesconnectionpage.cpp
    src/niixtorrent/ui/dialogs/preferencesdialog.cpp
    src/niixtorrent/ui/dialogs/preferencesdownloadspage.cpp
    src/niixtorrent/ui/dialogs/preferencesgeneralpage.cpp
    src/niixtorrent/ui/dialogs/preferenceslabelspage.cpp
    src/niixtorrent/ui/dialogs/preferencesproxypage.cpp
    src/niixtorrent/ui/dialogs/textoutputdialog.cpp

    # Filters
    src/niixtorrent/ui/filters/pqltorrentfilter.cpp

    # Models
    src/niixtorrent/ui/models/filestoragemodel.cpp
    src/niixtorrent/ui/models/peerlistmodel.cpp
    src/niixtorrent/ui/models/torrentlistmodel.cpp
    src/niixtorrent/ui/models/trackerlistmodel.cpp

    # Persistence
    src/niixtorrent/ui/persistence/persistenttorrentlistview.cpp

    # UI
    src/niixtorrent/ui/console.cpp
    src/niixtorrent/ui/icons.cpp
    src/niixtorrent/ui/mainframe.cpp
    src/niixtorrent/ui/statusbar.cpp
    src/niixtorrent/ui/taskbaricon.cpp
    src/niixtorrent/ui/theme.cpp
    src/niixtorrent/ui/torrentcontextmenu.cpp
    src/niixtorrent/ui/torrentdetailsfilespanel.cpp
    src/niixtorrent/ui/torrentdetailsoverviewpanel.cpp
    src/niixtorrent/ui/torrentdetailspeerspanel.cpp
    src/niixtorrent/ui/torrentdetailstrackerspanel.cpp
    src/niixtorrent/ui/torrentdetailsview.cpp
    src/niixtorrent/ui/torrentfilelistview.cpp
    src/niixtorrent/ui/torrentlistview.cpp
    src/niixtorrent/ui/translator.cpp

    # Widgets
    src/niixtorrent/ui/widgets/pieceprogressbar.cpp
    src/niixtorrent/ui/widgets/progressrenderer.cpp
    src/niixtorrent/ui/widgets/tabbook.cpp
)

if(WIN32)
    add_executable(NiiXTorrent WIN32 ${NIIX_SOURCES} src/niixtorrent/resources.rc)
else()
    add_executable(NiiXTorrent ${NIIX_SOURCES})
    set_target_properties(NiiXTorrent PROPERTIES OUTPUT_NAME niixtorrent)
endif()

niix_embed_directory(NiiXTorrent
    "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_migrations.cpp"
    Migrations "${CMAKE_CURRENT_SOURCE_DIR}/res/dbmigrations" "*.sql")
niix_embed_directory(NiiXTorrent
    "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_icons.cpp"
    Icons "${CMAKE_CURRENT_SOURCE_DIR}/res/icons" "*.*")

target_include_directories(NiiXTorrent PRIVATE
    include
    src/niixtorrent)

target_compile_definitions(NiiXTorrent PRIVATE
    LIBNIIX_API_EXPORT
    NIIX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NIIX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NIIX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    NIIX_GIT_COMMITISH=${NIIX_GIT_SHA})

target_link_libraries(NiiXTorrent PRIVATE
    Boost::boost
    Boost::log
    ${NIIX_WX_LIBS}
    fmt::fmt
    nlohmann_json::nlohmann_json
    LibtorrentRasterbar::torrent-rasterbar
    NiiXTorrentPQL
    ${NIIX_SQLITE_LIB})

if(WIN32)
    target_compile_definitions(NiiXTorrent PRIVATE
        _UNICODE UNICODE WIN32
        _WIN32_WINNT=0x0A00
        NOMINMAX
        WIN32_LEAN_AND_MEAN)
    target_link_libraries(NiiXTorrent PRIVATE
        Comctl32 crypt32 iphlpapi propsys shlwapi)
    if(MSVC)
        target_link_libraries(NiiXTorrent PRIVATE legacy_stdio_definitions)
    endif()
    if(MSVC)
        # Merged into the executable by the linker: per-monitor (v2) DPI awareness.
        target_sources(NiiXTorrent PRIVATE src/niixtorrent/application.manifest)
        target_compile_options(NiiXTorrent PRIVATE /guard:cf /W4)
        target_link_options(NiiXTorrent PRIVATE /guard:cf)
    endif()
else()
    target_compile_options(NiiXTorrent PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_definitions(NiiXTorrent PRIVATE
        NIIX_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/niixtorrent"
        NIIX_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/niixtorrent/plugins")

    # "Show in folder" talks to the desktop's file manager over D-Bus.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    target_link_libraries(NiiXTorrent PRIVATE PkgConfig::GIO PkgConfig::GTK3 ${CMAKE_DL_LIBS})
endif()

# Plugins link against the functions exported by the executable.
set_property(TARGET NiiXTorrent PROPERTY ENABLE_EXPORTS ON)

# --------------------------------------------------------------------------
# Plugins
# --------------------------------------------------------------------------
if(WIN32)
    # The update checker uses the Windows task dialog; on Linux updates are
    # delivered by Flatpak instead.
    add_library(Plugin_Updater SHARED src/plugins/updater/updater.cpp)
    target_compile_definitions(Plugin_Updater PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_include_directories(Plugin_Updater PRIVATE include)
    target_link_libraries(Plugin_Updater PRIVATE NiiXTorrent Comctl32)
endif()

# --------------------------------------------------------------------------
# Crashpad
# --------------------------------------------------------------------------
if(WITH_CRASHPAD)
    target_compile_definitions(NiiXTorrent PRIVATE NIIX_WITH_CRASHPAD)
    target_link_libraries(NiiXTorrent PRIVATE crashpad_client crashpad_util mini_chromium)
    add_custom_command(TARGET NiiXTorrent POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:crashpad_handler> $<TARGET_FILE_DIR:NiiXTorrent>)
endif()

# --------------------------------------------------------------------------
# Translation database
# --------------------------------------------------------------------------
add_custom_command(
    OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/coredb/coredb.sqlite"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/coredb"
    COMMAND NiiXTorrent-coredb "${CMAKE_CURRENT_SOURCE_DIR}/lang" "${CMAKE_CURRENT_BINARY_DIR}/coredb"
    DEPENDS NiiXTorrent-coredb ${NIIX_LANG_FILES}
    COMMENT "Generating translation database"
    VERBATIM)
# Also placed next to the executable so the app runs straight from the build
# tree. Done in its own target so a changed translation is picked up without
# relinking the application.
add_custom_target(NiiXTorrent-coredb-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/coredb/coredb.sqlite" $<TARGET_FILE_DIR:NiiXTorrent>
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/coredb/coredb.sqlite"
    VERBATIM)
add_dependencies(NiiXTorrent-coredb-data NiiXTorrent)

# --------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------
if(WIN32)
    # Flat layout consumed by packaging/windows/NiiXTorrent.iss and the
    # portable zip:  cmake --install <build> --prefix <dist>
    install(TARGETS NiiXTorrent RUNTIME DESTINATION .)
    install(TARGETS Plugin_Updater RUNTIME DESTINATION . LIBRARY DESTINATION .)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/coredb/coredb.sqlite" LICENSE DESTINATION .)
    if(WITH_CRASHPAD)
        install(PROGRAMS $<TARGET_FILE:crashpad_handler> DESTINATION .)
    endif()

    # Ship the Visual C++ runtime DLLs next to the executable so neither the
    # installer nor the portable zip depends on a separately installed
    # redistributable. (The Universal CRT is part of Windows 10 and later.)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
    include(InstallRequiredSystemLibraries)
else()
    set(NIIX_APP_ID "io.github.niixtorrent.NiiXTorrent")

    install(TARGETS NiiXTorrent RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/coredb/coredb.sqlite"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/niixtorrent)
    install(FILES packaging/linux/${NIIX_APP_ID}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/${NIIX_APP_ID}.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    foreach(_size 16 24 32 48 64 128 256)
        install(FILES packaging/linux/icons/${_size}x${_size}/${NIIX_APP_ID}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps)
    endforeach()
    install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/niixtorrent)
endif()
