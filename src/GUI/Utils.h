#pragma once

#include <ctime>

#include "../common.h"

namespace fs = std::filesystem;

class Utils {
    public:
        static std::string GetLastModifiedTime(const std::string& fpath);
        static std::string GetFileSize(const fs::path& path);
        static std::string ToLower(const std::string& str);
};