#include <Image.h>
#include <Utils.h>
#include <util/Logger.h>
#include <gles3/gl3.h>

#include <string_view>
#include <algorithm>

#include "../ResourceFormats/DDS.h"
#include "ResourceFormats/dds_formats.h"
using dds::ReadResult;

bool Image::LoadGifAnimation(void* data, size_t data_size, GifAnimation* out_animation)
{
    SDL_IOStream* stream = SDL_IOFromMem(data, data_size);
    if (!stream) {
        Logger::error("Failed to create IOStream: %s", SDL_GetError());
        return false;
    }

    IMG_Animation* animation = IMG_LoadAnimation_IO(stream, 1);
    if (!animation) {
        Logger::error("Failed to load GIF animation: %s", SDL_GetError());
        return false;
    }

    out_animation->frame_count = animation->count;
    out_animation->width = animation->w;
    out_animation->height = animation->h;

    out_animation->delays = (u32*)SDL_calloc(out_animation->frame_count, sizeof(u32));
    out_animation->total_duration_ms = 0;

    for (int i = 0; i < out_animation->frame_count; i++) {
        out_animation->delays[i] = animation->delays[i];
        out_animation->total_duration_ms += animation->delays[i];
    }

    for (int i = 0; i < out_animation->frame_count; i++) {
        out_animation->delays[i] = animation->delays[i];
    }

    for (int i = 0; i < animation->count; i++) {
        SDL_Surface* frame_surface = animation->frames[i];
        if (!frame_surface) {
            Logger::error("Failed to load GIF frame %d: %s", i, SDL_GetError());
            continue;
        }

        SDL_Surface* converted_surface = SDL_ConvertSurface(frame_surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(frame_surface);

        if (!converted_surface) {
            Logger::error("Failed to convert GIF frame %d", i);
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted_surface->w, converted_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted_surface->pixels);

        out_animation->frames.push_back(texture);
        SDL_DestroySurface(converted_surface);
    }

    IMG_FreeAnimation(animation);
    return true;
}

GLuint Image::GetGifFrame(const GifAnimation& animation, int *frame_index)
{
    if (animation.frames.empty()) return 0;

    *frame_index = *frame_index % animation.frame_count;
    return animation.frames[*frame_index];
}

void Image::UnloadAnimation(GifAnimation* animation)
{
    for (GLuint texture : animation->frames) {
        glDeleteTextures(1, &texture);
    }
    animation->frames.clear();
    animation->frame_count = 0;
    return;
}

GLuint Image::LoadTex(const u8* data, int width, int height, u32 mode) {
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);

    // glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::log("OpenGL error uploading texture: 0x%x", err);
    }
    return image_texture;
}

bool Image::LoadImage(void* data, size_t data_size, GLuint *out_texture, Vec2<int*> out_size, u32 mode) {
    dds::Image image;
    auto result = dds::readImage((u8*)data, data_size, &image);
    if (result == ReadResult::Success) {
        auto id = Image::LoadTex(image.mipmaps[0].data(), image.width, image.height);
        *out_texture = id;
        *out_size.x = image.width;
        *out_size.y = image.height;

        return true;
    } else if (result != ReadResult::InvalidMagic) {
        Logger::log("Failed to load DDS into memory!");
        Logger::log("Error: %s", dds::DecodeReadResult(result).c_str());
    }

    int image_width = 0;
    int image_height = 0;
    u8 *image_data;

    SDL_IOStream *stream = SDL_IOFromMem(data, data_size);
    if (!stream) {
        Logger::error("Failed to create IOStream: %s", SDL_GetError());
        return false;
    }
    SDL_Surface *surface = IMG_Load_IO(stream, 1);
    if (!surface) {
        Logger::error("Failed to load image: %s", SDL_GetError());
        return false;
    }

    SDL_Surface *converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!converted_surface) {
        Logger::error("Failed to convert surface format: %s", SDL_GetError());
        return false;
    }

    image_width = converted_surface->w;
    image_height = converted_surface->h;
    image_data = (u8*)(converted_surface->pixels);

    GLuint image_texture = LoadTex(image_data, image_width, image_height, mode);

    SDL_DestroySurface(converted_surface);

    *out_texture = image_texture;
    *out_size.x = image_width;
    *out_size.y = image_height;

    return true;
}

bool Image::UnloadTexture(GLuint texture)
{
    if (texture != 0) {
        glDeleteTextures(1, &texture);
        return true;
    }

    return false;
}

bool Image::IsGif(std::string_view ext) {
    return ext == "gif";
}

const std::string image_exts[] = {"png", "jpg", "jpeg", "bmp", "webp", "svg", "tga", "tif", "tiff", "jxl", "tlg", "dds"};

bool Image::IsImageExtension(const std::string &ext)
{
    std::string ext_lower = Utils::ToLower(ext);
    return std::find(std::begin(image_exts), std::end(image_exts), ext_lower) != std::end(image_exts);
}
