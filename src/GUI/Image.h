#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "gl3.h"
#include "../vec2.h"

struct GifAnimation {
    std::vector<GLuint> frames;
    int frame_count = 0;
    int width;
    int height;
    int *delays;
};

class Image {
    public:
        static bool LoadImage(const void* data, size_t data_size, GLuint *out_texture, Vec2<int*> out_size, uint32_t mode = GL_LINEAR);
        static bool LoadGifAnimation(const void* data, size_t data_size, GifAnimation* out_animation);
        static bool UnloadTexture(GLuint texture);
        static void UnloadAnimation(GifAnimation* animation);
        static bool IsGif(const std::string &ext);
        static GLuint GetGifFrame(const GifAnimation& animation, int *frame_index);
        static bool IsImageExtension(const std::string& ext);
};
