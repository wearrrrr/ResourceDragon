#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "gl3.h"
#include "../vec2.h"
#include <util/int.h>

struct GifAnimation {
    std::vector<GLuint> frames;
    int frame_count = 0;
    int width;
    int height;
    int *delays;
};

namespace Image {
    GLuint LoadTex(const u8* data, int width, int height, u32 mode = GL_LINEAR);
    bool LoadImage(const void* data, size_t data_size, GLuint *out_texture, Vec2<int*> out_size, u32 mode = GL_LINEAR);
    bool LoadGifAnimation(const void* data, size_t data_size, GifAnimation* out_animation);
    bool UnloadTexture(GLuint texture);
    void UnloadAnimation(GifAnimation* animation);
    bool IsGif(const std::string &ext);
    GLuint GetGifFrame(const GifAnimation& animation, int *frame_index);
    bool IsImageExtension(const std::string& ext);
};
