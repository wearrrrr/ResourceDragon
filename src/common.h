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

struct PreviewWinState {
    char *rawContents = nullptr;
    long rawContentsSize = 0;
    std::string rawContentsExt;
    struct {
        GifAnimation anim;
        int frame;
        int last_frame_time;
        GLuint id;
        struct {
            int x;
            int y;
        } size;
    } texture;
};

inline PreviewWinState preview_state = {
    .rawContents = nullptr,
    .rawContentsSize = 0,
    .rawContentsExt = "",

    .texture = {
        .anim = {},
        .frame = 0,
        .last_frame_time = 0,
        .id = 0,
        .size = {0, 0}
    }
};
inline ExtractorManager extractor_manager;

