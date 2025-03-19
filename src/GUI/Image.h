#pragma once

#include <string>
#include <GLES3/gl3.h>

class Image {
    public:
        static bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height);
        static bool UnloadTexture(GLuint texture);
        static bool IsImageExtension(const std::string& ext);
};