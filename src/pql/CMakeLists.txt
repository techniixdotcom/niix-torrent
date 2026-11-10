# PQL - the NiiX Torrent query language (ANTLR4 grammar in Query.g4).
#
# The parser in generated/ was produced with ANTLR 4.13.2 and must be built
# against the matching runtime. The runtime is taken from vendor/antlr4 when
# that checkout exists (scripts/bootstrap.*), otherwise from an installed
# antlr4-runtime package.

set(NIIX_ANTLR4_DIR "${CMAKE_SOURCE_DIR}/vendor/antlr4" CACHE PATH "ANTLR4 source checkout")
set(_antlr_runtime "${NIIX_ANTLR4_DIR}/runtime/Cpp/runtime/src")

if(EXISTS "${_antlr_runtime}/antlr4-runtime.h")
    file(GLOB_RECURSE _antlr_sources CONFIGURE_DEPENDS "${_antlr_runtime}/*.cpp")

    add_library(niix_antlr4 STATIC ${_antlr_sources})
    target_compile_definitions(niix_antlr4 PUBLIC ANTLR4CPP_STATIC)
    target_include_directories(niix_antlr4 PUBLIC "${_antlr_runtime}")
    target_compile_features(niix_antlr4 PUBLIC cxx_std_17)
    if(MSVC)
        target_compile_options(niix_antlr4 PRIVATE /W0)
    else()
        target_compile_options(niix_antlr4 PRIVATE -w)
        find_package(Threads REQUIRED)
        target_link_libraries(niix_antlr4 PUBLIC Threads::Threads)
    endif()
    set(_antlr_target niix_antlr4)
    message(STATUS "NiiX: using vendored ANTLR4 runtime (${NIIX_ANTLR4_DIR})")
else()
    find_package(antlr4-runtime 4.13.2 CONFIG REQUIRED)
    if(TARGET antlr4_static)
        set(_antlr_target antlr4_static)
    else()
        set(_antlr_target antlr4_shared)
    endif()
    message(STATUS "NiiX: using installed ANTLR4 runtime ${antlr4-runtime_VERSION}")
endif()

add_library(NiiXTorrentPQL STATIC
    generated/QueryBaseVisitor.cpp
    generated/QueryLexer.cpp
    generated/QueryParser.cpp
    generated/QueryVisitor.cpp)

target_include_directories(NiiXTorrentPQL INTERFACE generated)
target_link_libraries(NiiXTorrentPQL PUBLIC ${_antlr_target})

if(TARGET antlr4_static OR TARGET antlr4_shared)
    # Installed runtimes put their headers in <prefix>/include/antlr4-runtime.
    if(ANTLR4_INCLUDE_DIR)
        target_include_directories(NiiXTorrentPQL PUBLIC "${ANTLR4_INCLUDE_DIR}")
    endif()
endif()

if(NOT MSVC)
    target_compile_options(NiiXTorrentPQL PRIVATE -w)
endif()
