#include <Audio.h>
#include <Utils.h>
#include <DirectoryNode.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <algorithm>

#include "state.h"

void Audio::MusicFinishedCallback(void* userdata, MIX_Track *track) {
    const PreviewWinState &state = GetPreviewState(preview_index);
    if (state.audio.music && state.audio.shouldLoop) {
        MIX_PlayTrack(state.audio.track, 0);
    }
}

void Audio::InitAudioSystem() {
    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = 2,
        .freq = 48000,
    };

    MIX_Init();

    PreviewWinState &state = GetPreviewState(preview_index);
    state.audio.mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!state.audio.mixer) {
        Logger::error("Failed to initialize SDL_mixer: {}", SDL_GetError());
        return;
    }

    MIX_GetMixerFormat(state.audio.mixer, &spec);
    if (spec.freq == 0 || spec.format == 0 || spec.channels == 0) {
        Logger::error("Failed to query audio spec: {}", SDL_GetError());
        MIX_DestroyAudio(state.audio.music);
        return;
    }

    state.audio.track = MIX_CreateTrack(state.audio.mixer);
    if (!state.audio.track) {
        Logger::error("Failed to reserve track: {}", SDL_GetError());
        return;
    }

    MIX_SetTrackStoppedCallback(state.audio.track, MusicFinishedCallback, nullptr);
    return;
}

const std::string audio_exts[] = {
    "wav", "mp3", "ogg", "flac", "aac", "opus",
    "m4a", "wma"
};

bool Audio::IsAudio(const std::string &ext) {
    return std::find(std::begin(audio_exts), std::end(audio_exts), Utils::ToLower(ext)) != std::end(audio_exts);
};
