#pragma once

#define MIDI_SUPPORT

#include <SDL3_mixer/SDL_mixer.h>
#include <filesystem>

#include "Utils.h"

namespace fs = std::filesystem;

inline Mix_Music *current_sound = nullptr;
inline bool curr_sound_is_midi = false;
namespace Audio {
    void InitAudioSystem();
    bool IsAudio(const std::string &ext);
    bool PlaySound(const fs::path &path);
};