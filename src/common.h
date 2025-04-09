#pragma once

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <string>
#include <vector>

#ifdef linux
#include <sys/inotify.h>
#define EVENT_SIZE  (sizeof (struct inotify_event))
#include <thread>
#endif

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "ArchiveFormats/HSP/hsp.h"
#include "ArchiveFormats/XP3/xp3.h"
#include "ExtractorManager.h"

#include "GUI/Audio.h"
#include "GUI/Clipboard.h"
#include "GUI/DirectoryNode.h"
#include "GUI/Image.h"
#include "GUI/TextEditor/TextEditor.h"
#include "GUI/Theme/Themes.h"
#include "GUI/Utils.h"



#include "util/Logger.h"

struct PWinStateTexture {
    GLuint id;
    struct {
        int x;
        int y;
    } size;
    GifAnimation anim;
    int frame;
    int last_frame_time;
};
struct TimeInfo {
    int total_time_min;
    int total_time_sec;
    int current_time_min;
    int current_time_sec;
};
struct PWinStateAudio {
    Mix_Music *music;
    bool playing;
    int volume;
    TimeInfo time;
    SDL_TimerID update_timer;
    int fade_steps;
    int fade_step;
    Mix_Fading fading;
};
struct PreviewWinState {
    std::string content_type;
    struct {
        std::string data;
        std::string path;
        std::string ext;
    } contents;
    PWinStateAudio audio;
    PWinStateTexture texture;
};
struct UIError {
    std::string message;
    std::string title;
    bool show = false;
};

inline PreviewWinState preview_state = {
    .content_type = "",
    .contents = {
        .data = "",
        .path = "",
        .ext = "",
    },
    .audio = {
        .music = nullptr,
        .playing = false,
        .volume = 0,
        .time = {
            .total_time_min = 0,
            .total_time_sec = 0,
            .current_time_min = 0,
            .current_time_sec = 0
        },
        .update_timer = 0,
    },
    .texture = {
        .id = 0,
        .size = {0, 0},
        .anim = {},
        .frame = 0,
        .last_frame_time = 0,
    }
};
inline ExtractorManager extractor_manager;

inline TextEditor editor;

inline UIError ui_error = {
    .message = "",
    .title = "",
    .show = false
};

#ifdef linux
inline int inotify_fd;
inline int inotify_wd;
#endif

