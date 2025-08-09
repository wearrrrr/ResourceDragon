#include <Audio.h>
#include <Utils.h>
#include <DirectoryNode.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <algorithm>

#include "state.h"

void Audio::MusicFinishedCallback(void* userdata, MIX_Track *track) {
    if (preview_state.audio.music && preview_state.audio.shouldLoop) {
        MIX_PlayTrack(preview_state.audio.track, 0);
    }
}

void Audio::InitAudioSystem() {
    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = 2,
        .freq = 48000,
    };

    MIX_Init();

    preview_state.audio.mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!preview_state.audio.mixer) {
        Logger::error("Failed to initialize SDL_mixer: %s", SDL_GetError());
        return;
    }

    MIX_GetMixerFormat(preview_state.audio.mixer, &spec);
    if (spec.freq == 0 || spec.format == 0 || spec.channels == 0) {
        Logger::error("Failed to query audio spec: %s", SDL_GetError());
        MIX_DestroyAudio(preview_state.audio.music);
        return;
    }

    preview_state.audio.track = MIX_CreateTrack(preview_state.audio.mixer);
    if (!preview_state.audio.track) {
        Logger::error("Failed to reserve track: %s", SDL_GetError());
        return;
    }

    MIX_SetTrackStoppedCallback(preview_state.audio.track, MusicFinishedCallback, nullptr);
    return;
}

const std::string audio_exts[] = {
    "wav", "mp3", "ogg", "flac", "aac", "opus",
    "m4a", "wma"
};

bool Audio::IsAudio(const std::string &ext) {
    return std::find(std::begin(audio_exts), std::end(audio_exts), Utils::ToLower(ext)) != std::end(audio_exts);
};
