#pragma once
#include <SDL3_mixer/SDL_mixer.h>
#include "imgui.h"
#include "state.h"

namespace PreviewWindow {
    inline float timeToSetOnRelease = 0.0f;
    void RenderImagePreview();
    void RenderGifPreview();
    void RenderAudioPlayer();
    void RenderElfPreview();
    void RenderTextViewer(ImGuiIO &io);

    inline void RenderPreviewFor(ContentType content_type) {
        switch (content_type) {
            case IMAGE:
                RenderImagePreview();
                break;
            case GIF:
                RenderGifPreview();
                break;
            case AUDIO:
                RenderAudioPlayer();
                break;
            case ELF:
                RenderElfPreview();
                break;
            default:
                RenderTextViewer(ImGui::GetIO());
                break;
        }
    };
};
