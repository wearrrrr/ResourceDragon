#pragma once

#include <string>
#include <SDL3_mixer/SDL_mixer.h>

namespace Audio {
    void InitAudioSystem();
    bool IsAudio(const std::string &ext);

    void MusicFinishedCallback(void* userdata, MIX_Track *track);
};
