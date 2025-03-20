#pragma once

#include <filesystem>
#include <string>
#include <cstring>
#include <ctime>
#include <vector>

#include "iconv.h"

namespace fs = std::filesystem;

class Utils {
    public:
        static std::string GetLastModifiedTime(const std::string& fpath);
        static std::string GetFileSize(const fs::path& path);
        static std::string ShiftJISToUTF8(const std::string& sjis);
};