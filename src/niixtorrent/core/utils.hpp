#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace pt
{
    class Utils
    {
    public:
        // Opens the platform file manager with the given file or folder selected.
        static void openAndSelect(std::filesystem::path path);

        // "1.42 GB", "598 bytes", ... (binary units, like Windows Explorer).
        static std::wstring toHumanFileSize(int64_t bytes);

        // UTF-8 <-> wide conversions.
        static std::string toStdString(std::wstring const& input);
        static std::wstring toStdWString(std::string const& input);

        // UTF-8 path string, independent of the process locale.
        static std::string toUtf8(std::filesystem::path const& path);
        static std::filesystem::path fromUtf8(std::string const& utf8);

        // Full path of the running executable.
        static std::filesystem::path executablePath();

        // Fraction (0..1) of free space on the volume holding `path`, or a
        // negative value if it cannot be determined.
        static double freeDiskSpaceRatio(std::filesystem::path const& path);
    };
}
