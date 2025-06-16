#include <Audio.h>
#include <Utils.h>
#include <DirectoryNode.h>
#include "state.h"

void Audio::MusicFinishedCallback() {
    if (preview_state.audio.music && preview_state.audio.shouldLoop) {
        Mix_PlayMusic(preview_state.audio.music, 0);
    } else {
        UnloadSelectedFile();
    }
}

void Audio::InitAudioSystem() {
    SDL_AudioSpec spec;
    spec.channels = MIX_DEFAULT_CHANNELS;
    spec.format = MIX_DEFAULT_FORMAT;
    spec.freq = MIX_DEFAULT_FREQUENCY;

    #ifdef MIDI_SUPPORT
    bool load_sf2 = Mix_SetSoundFonts("opl4.sf2");
    if (!load_sf2) {
        Logger::error("Failed to load soundfont: %s", SDL_GetError());
    }
    #endif


    if (!Mix_OpenAudio(0, &spec)) {
        Logger::error("Failed to initialize SDL_mixer: %s", SDL_GetError());
        return;
    }

    Mix_AllocateChannels(16);

    Mix_QuerySpec(&spec.freq, &spec.format, &spec.channels);
    if (spec.freq == 0 || spec.format == 0 || spec.channels == 0) {
        Logger::error("Failed to query audio spec: %s", SDL_GetError());
        Mix_CloseAudio();
        return;
    }

    Mix_HookMusicFinished(MusicFinishedCallback);
    return;
}

const std::string audio_exts[] = {
    "wav", "mp3", "ogg", "flac", "aac", "opus",
    "m4a", "wma"
};

bool Audio::IsAudio(const std::string &ext)
{
    std::string ext_lower = Utils::ToLower(ext);
    if (ext_lower == "mid" || ext_lower == "midi") {
        curr_sound_is_midi = true;
        return true;
    }
    curr_sound_is_midi = false;

    return std::find(std::begin(audio_exts), std::end(audio_exts), ext_lower) != std::end(audio_exts);
};

bool Audio::PlaySound(const fs::path &path) {
    current_sound = Mix_LoadMUS(path.string().c_str());
    if (!current_sound) {
        Logger::error("Failed to load Audio: %s", SDL_GetError());
        // Setting to nullptr just in case
        // This just makes sure that if the music fails to load, it won't try to play anyways.
        current_sound = nullptr;
        return false;
    }
    if (!Mix_PlayMusic(current_sound, 0)) {
        Logger::error("Failed to play music: %s", SDL_GetError());
        Mix_FreeMusic(current_sound);
        current_sound = nullptr;
        return false;
    }
    return true;
};
