#pragma once

#include <filesystem>
#include <string>
#include <cstring>
#include <ctime>
#include <vector>

#include "iconv.h"
#include <GLES3/gl3.h>

class Utils {
    public:
        static std::string GetLastModifiedTime(const std::string& fpath);
        static std::string GetFileSize(const std::filesystem::path& path);
        static std::string ShiftJISToUTF8(const std::string& sjis);
};

bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height);