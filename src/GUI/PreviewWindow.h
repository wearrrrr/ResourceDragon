#pragma once
#include <SDL3_mixer/SDL_mixer.h>
#include <imgui.h>
#include "state.h"

#define SIZEOF_ARRAY(arr) sizeof(arr) / sizeof(arr[0])

namespace PreviewWindow {
    inline float timeToSetOnRelease = 0.0f;
    void RenderImagePreview();
    void RenderGifPreview();
    void RenderAudioPlayer();
    void RenderElfPreview();
    void RenderTextViewer(ImGuiIO &io);
    void RenderHexEditor(ImGuiIO &io);

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
                if (preview_state.show_hex) {
                    RenderHexEditor(ImGui::GetIO());
                } else {
                    RenderTextViewer(ImGui::GetIO());
                }
                break;
        }
    };
};
