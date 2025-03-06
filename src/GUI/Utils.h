#pragma once

#include <filesystem>
#include <string>
#include <cstring>
#include <ctime>
#include <vector>

#include "iconv.h"

class Utils {
    public:
        static std::string GetLastModifiedTime(const std::string& fpath);
        static std::string GetFileSize(const std::filesystem::path& path);
        static std::string ShiftJISToUTF8(const std::string& sjis);
};