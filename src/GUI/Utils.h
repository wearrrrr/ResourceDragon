#pragma once

#include <filesystem>

namespace fs = std::filesystem;

class Utils {
    public:
        static std::string GetLastModifiedTime(const std::string& path);
        static std::string GetFileSize(const fs::path& path);
        static std::string GetFileSize(uint32_t size);
        static std::string ToLower(const std::string &str);
};
