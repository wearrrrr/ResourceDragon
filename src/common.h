#pragma once

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "ArchiveFormats/HSP/hsp.h"
#include "ExtractorManager.h"

#include "GUI/Theme/Themes.h"
#include "GUI/DirectoryNode.h"
#include "GUI/Image.h"
#include "GUI/Utils.h"

#include "util/Logger.h"

struct PWinStateTexture {
    GifAnimation anim;
    int frame;
    int last_frame_time;
    GLuint id;
    struct {
        int x;
        int y;
    } size;
};
struct PreviewWinState {
    struct {
        char *data;
        long size;
        std::string ext;
    } contents;
    PWinStateTexture texture;
};



inline PreviewWinState preview_state = {
    .contents = {
        .data = nullptr,
        .size = 0,
        .ext = "",
    },
    .texture = {
        .anim = {},
        .frame = 0,
        .last_frame_time = 0,
        .id = 0,
        .size = {0, 0}
    }
};
inline ExtractorManager extractor_manager;

