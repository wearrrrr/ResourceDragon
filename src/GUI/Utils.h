#pragma once

#include <filesystem>
#include <util/int.h>

namespace fs = std::filesystem;

class Utils {
    public:
        static std::string GetLastModifiedTime(const std::string& path);
        static std::string GetFileSize(const fs::path& path);
        static std::string GetFileSize(u64 size);
        static std::string ToLower(const std::string &str);
};
