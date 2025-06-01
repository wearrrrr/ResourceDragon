#pragma once
#include <SDL3_mixer/SDL_mixer.h>
#include "imgui.h"

namespace PreviewWindow {
    inline float timeToSetOnRelease = 0.0f;
    void RenderImagePreview();
    void RenderAudioPlayer();
    void RenderTextViewer(ImGuiIO &io);
};
