#include "Audio.h"

void Audio::InitAudioSystem() {
    SDL_AudioSpec spec;
    spec.channels = MIX_DEFAULT_CHANNELS;
    spec.format = MIX_DEFAULT_FORMAT;
    spec.freq = MIX_DEFAULT_FREQUENCY;

    #ifdef MIDI_SUPPORT
    Mix_SetSoundFonts("8bitsf.SF2");
    #endif


    if (!Mix_OpenAudio(0, &spec)) {
        SDL_Log("Failed to initialize SDL_mixer: %s", SDL_GetError());
        return;
    }

    Mix_AllocateChannels(16);
    
    Mix_QuerySpec(&spec.freq, &spec.format, &spec.channels);
    if (spec.freq == 0 || spec.format == 0 || spec.channels == 0) {
        SDL_Log("Failed to query audio spec: %s", SDL_GetError());
        Mix_CloseAudio();
        return;
    }
}
bool Audio::IsAudio(const fs::path &path)
{
    std::string ext = path.extension().string();

    if (ext == ".mid" || ext == ".midi") {
        curr_sound_is_midi = true;
        return true;
    }

    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || 
        ext == ".flac" || ext == ".aac" || ext == ".opus" ||
        ext == ".m4a" || ext == ".wma" || ext == ".mid" || ext == ".midi") {

        return true;
    }
    return false;
};

bool Audio::PlaySound(const fs::path &path) {
    current_sound = Mix_LoadMUS(path.c_str());
    if (!current_sound) {
        SDL_Log("Failed to load Audio: %s", SDL_GetError());
        // Setting to nullptr just in case
        // This just makes sure that if the music fails to load, it won't try to play anyways.
        current_sound = nullptr;
        return false;
    }
    if (!Mix_PlayMusic(current_sound, 1)) {
        SDL_Log("Failed to play music: %s", SDL_GetError());
        Mix_FreeMusic(current_sound);
        current_sound = nullptr;
        return false;
    }
    return true;
};