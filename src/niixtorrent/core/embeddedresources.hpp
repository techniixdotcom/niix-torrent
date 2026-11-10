#pragma once

#include <cstddef>

namespace pt
{
namespace Core
{
    // A file compiled into the executable by cmake/EmbedResources.cmake.
    // Tables returned below are terminated by an entry whose name is nullptr.
    struct EmbeddedResource
    {
        const char*          name;   // file name without extension
        const unsigned char* data;
        std::size_t          size;
    };

    // SQL migrations from res/dbmigrations, sorted by file name.
    const EmbeddedResource* EmbeddedMigrations();

    // PNG images from res/icons.
    const EmbeddedResource* EmbeddedIcons();
}
}
