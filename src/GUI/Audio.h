#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <filesystem>

namespace fs = std::filesystem;

inline Mix_Music *current_sound = nullptr;
namespace Audio {
    void InitAudioSystem();
    bool IsAudio(const std::string &ext);
    bool PlaySound(const fs::path &path);

    void MusicFinishedCallback();
};
