#include "Image.h"

bool Image::LoadGifAnimation(const void* data, size_t data_size, GifAnimation* out_animation)
{
    SDL_IOStream* stream = SDL_IOFromConstMem(data, data_size);
    if (!stream) {
        SDL_Log("Failed to create IOStream: %s", SDL_GetError());
        return false;
    }

    IMG_Animation* animation = IMG_LoadAnimation_IO(stream, 1);
    if (!animation) {
        SDL_Log("Failed to load GIF animation: %s", SDL_GetError());
        return false;
    }

    out_animation->frame_count = animation->count;
    out_animation->width = animation->w;
    out_animation->height = animation->h;

    out_animation->delays = (int*)SDL_calloc(out_animation->frame_count, sizeof(int));

    for (int i = 0; i < out_animation->frame_count; i++) {
        out_animation->delays[i] = animation->delays[i];
    }

    for (int i = 0; i < animation->count; i++) {
        SDL_Surface* frame_surface = animation->frames[i];
        if (!frame_surface) {
            SDL_Log("Failed to load GIF frame %d: %s", i, SDL_GetError());
            continue;
        }

        SDL_Surface* converted_surface = SDL_ConvertSurface(frame_surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(frame_surface);

        if (!converted_surface) {
            SDL_Log("Failed to convert GIF frame %d", i);
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
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


bool Image::LoadImageSDL(const void* data, size_t data_size, GLuint *out_texture, int *out_width, int *out_height) {
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data;

    SDL_IOStream *stream = SDL_IOFromConstMem(data, data_size);
    if (!stream) {
        SDL_Log("Failed to create IOStream: %s", SDL_GetError());
        return false;
    }
    SDL_Surface *surface = IMG_Load_IO(stream, 1);
    if (!surface) {
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return false;
    }

    SDL_Surface *converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!converted_surface) {
        SDL_Log("Failed to convert surface format: %s", SDL_GetError());
        return false;
    }

    image_width = converted_surface->w;
    image_height = converted_surface->h;
    image_data = (unsigned char *)(converted_surface->pixels);

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    SDL_DestroySurface(converted_surface);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

bool Image::LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    LoadImageSDL(data, data_size, out_texture, out_width, out_height);

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

bool Image::IsGif(const std::string &ext) {
    return ext == "gif";
}

const std::string image_exts[] = {"png", "jpg", "jpeg", "bmp", "webp", "svg", "tga", "tif", "tiff", "jxl"};

bool Image::IsImageExtension(const std::string &ext)
{
    return std::find(std::begin(image_exts), std::end(image_exts), ext) != std::end(image_exts);
}
