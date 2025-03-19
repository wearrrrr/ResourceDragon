#pragma once

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <cmath>

#include "imgui.h"
#include "../vendored/imgui/imgui_impl_sdl3.h"
#include "../vendored/imgui/imgui_impl_opengl3.h"

#include "ArchiveFormats/HSP/hsp.h"
#include "ExtractorManager.h"

#include "GUI/Theme/Themes.h"
#include "GUI/DirectoryNode.h"
#include "GUI/Image.h"
#include "GUI/Utils.h"

struct PreviewWinState {
    char *rawContents = nullptr;
    long rawContentsSize = 0;
    std::string rawContentsExt;
    struct {
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
        .id = 0,
        .size = {0, 0}
    }
};
inline const char *selectedItem;
inline ExtractorManager extractor_manager;

