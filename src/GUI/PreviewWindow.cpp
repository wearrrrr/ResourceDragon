#include <PreviewWindow.h>
#include <Audio.h>
#include <DirectoryNode.h>
#include <ImVec2Util.h>
#include <fstream>
#include "../util/Text.h"
#include "../state.h"
#include "../icons.h"

void PreviewWindow::RenderImagePreview() {
    PWinStateTexture *texture = &preview_state.texture;
    ImGui::Text("Zoom: %.2fx", img_preview__zoom);
    ImVec2 region_size = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("ImageRegion", region_size, false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

    if (ImGui::IsWindowHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

    // zooming
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
        const float wheel = ImGui::GetIO().MouseWheel;
        const float prev_zoom = img_preview__zoom;
        img_preview__zoom = std::clamp(img_preview__zoom + wheel * 0.1f, 0.1f, 5.0f);

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const ImVec2 cursor_screen = ImGui::GetCursorScreenPos();
        const ImVec2 rel = mouse - cursor_screen - img_preview__pan;
        img_preview__pan -= rel * (img_preview__zoom / prev_zoom - 1.0f);
    }

    // dragging
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        img_preview__pan += delta;
    }

    if (texture->id) {
        ImVec2 image_size = ImVec2(texture->size.x * img_preview__zoom, texture->size.y * img_preview__zoom);
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        if (preview_state.texture.firstFrame) {
            img_preview__pan = {(region_size.x - image_size.x) * 0.5f, 0.0f};
            img_preview__zoom = 1.0f;
            preview_state.texture.firstFrame = false;
        }
        ImVec2 draw_pos = Floor(cursor + img_preview__pan);
        ImVec2 draw_end = draw_pos + Floor(image_size);
        ImGui::GetWindowDrawList()->AddImage(texture->id, draw_pos, draw_end);
    } else {
        ImGui::Text("Failed to load image!");
    }
}

void PreviewWindow::RenderGifPreview() {
    PWinStateTexture *texture = &preview_state.texture;
    GifAnimation &anim = texture->anim;
    ImVec2 image_size = ImVec2(anim.width, anim.height);
    ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - image_size.x) * 0.5f, 50));
    ImGui::Image(Image::GetGifFrame(anim, &texture->frame), image_size);

    uint32_t now = SDL_GetTicks();
    uint32_t frame_delay = (uint32_t)anim.delays[texture->frame];
    if (now - texture->last_frame_time >= frame_delay) {
        texture->frame = (texture->frame + 1) % anim.frame_count;
        texture->last_frame_time = now;
    }
}

bool PlaybackScrubber(const char *id, float *progress, float width, float height = 16.0f) {
    ImGui::PushID(id);

    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    const ImVec2 size = {width, height};

    ImGui::InvisibleButton("##scrubber", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    if (hovered && ImGui::IsMouseDown(0)) {
        float mouseX = ImGui::GetIO().MousePos.x;
        *progress = std::clamp((mouseX - cursorPos.x) / width, 0.0f, 1.0f);
    }
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
    drawList->AddRectFilled(cursorPos, {cursorPos.x + size.x, cursorPos.y + size.y}, bgColor, height * 0.5f);

    float fillWidth = width * (*progress);
    ImVec2 fillEnd = ImVec2(cursorPos.x + fillWidth, cursorPos.y + height);
    ImU32 fillColor = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
    drawList->AddRectFilled(cursorPos, fillEnd, fillColor, height * 0.5f);

    float knobRadius = height * 0.75f;
    ImVec2 knobCenter = ImVec2(cursorPos.x + fillWidth, cursorPos.y + height * 0.5f);
    ImU32 knobColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    drawList->AddCircleFilled(knobCenter, knobRadius, knobColor);

    ImGui::PopID();
    return active;
}


void PreviewWindow::RenderAudioPlayer() {
    if (current_sound) {
        ImGui::Text("Playing: %s", preview_state.contents.path.c_str());
        TimeInfo time = preview_state.audio.time;
        if (preview_state.audio.playing) {
            if (ImGui::Button(PAUSE_ICON, {40, 0})) {
                Mix_PauseMusic();
                preview_state.audio.playing = false;
            }
        } else {
            if (ImGui::Button(PLAY_ICON, {40, 0})) {
                Mix_ResumeMusic();
                preview_state.audio.playing = true;
            }
        }
        ImGui::SameLine();
        ImGui::BeginGroup();
        // Time info breaks if the audio file is a midi file, pretty sure this is unfixable?
        if (!curr_sound_is_midi) {
            if (ImGui::Button(RW_ICON, {40, 0})) {
                double new_pos = Mix_GetMusicPosition(current_sound) - 5.0;
                new_pos > 0
                    ? Mix_SetMusicPosition(new_pos)
                    : Mix_SetMusicPosition(0);
            }
            ImGui::SameLine();
            if (!curr_sound_is_midi) {
                ImGui::Text("%02d:%02d / %02d:%02d",
                    time.current_time_min,
                    time.current_time_sec,
                    time.total_time_min,
                    time.total_time_sec
                );
                ImGui::SameLine();
            }
            const double current_pos = Mix_GetMusicPosition(current_sound);
            const double total_time = time.total_time_min * 60 + time.total_time_sec;
            if (total_time > 0.0) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

                const float visual_pos = std::max(current_pos - 0.3, 0.0);
                float scrubberProgress = visual_pos / total_time;
                scrubberProgress = std::clamp(scrubberProgress, 0.0f, 1.0f);
                bool isDragging = PlaybackScrubber("AudioScrubber", &scrubberProgress, (ImGui::GetWindowWidth() / 2.0f));

                if (isDragging) {
                    if (!preview_state.audio.scrubberDragging) Mix_PauseMusic();

                    int new_pos = (int)(scrubberProgress * total_time);
                    timeToSetOnRelease = new_pos;
                    preview_state.audio.time.current_time_min = new_pos / 60;
                    preview_state.audio.time.current_time_sec = new_pos % 60;
                    preview_state.audio.scrubberDragging = true;
                }

                if (preview_state.audio.scrubberDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    Mix_SetMusicPosition(timeToSetOnRelease);
                    Mix_ResumeMusic();
                    preview_state.audio.scrubberDragging = false;
                }
            }
            ImGui::SameLine(0.0f, 16.0f);
            if (ImGui::Button(FF_ICON, {40, 0})) {
                double new_pos = Mix_GetMusicPosition(current_sound) + 5.0;
                Mix_SetMusicPosition(new_pos);
            }
            ImGui::EndGroup();
        }

        ImGui::SameLine();
        bool looping = preview_state.audio.shouldLoop;

        if (looping) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(117, 255, 154, 255));
        if (ImGui::Button(LOOP_ICON, {40, 0})) {
            preview_state.audio.shouldLoop = !preview_state.audio.shouldLoop;
        }
        if (looping) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button(STOP_ICON, {40, 0})) {
            Mix_HaltMusic();
            UnloadSelectedFile();
            preview_state.audio.playing = false;
            SDL_RemoveTimer(preview_state.audio.update_timer);
            preview_state.audio.update_timer = 0;
            preview_state.audio.time = {
                .total_time_min = 0,
                .total_time_sec = 0,
                .current_time_min = 0,
                .current_time_sec = 0,
            };
        }

        if (ImGui::SliderInt("Music Volume", &preview_state.audio.volumePercent, 0, 100, "%d%%")) {
            // Convert from percent (0–100) to SDL volume (0–128)
            int sdlVolume = (int)((preview_state.audio.volumePercent / 100.0f) * MIX_MAX_VOLUME);
            Mix_VolumeMusic(sdlVolume);
        }

        std::string titleTag = std::string(Mix_GetMusicTitleTag(current_sound));
        if (Text::trim(titleTag) != "") {
            ImGui::Text("Title: %s", titleTag.c_str());
        }
        std::string authorTag = std::string(Mix_GetMusicArtistTag(current_sound));
        if (!(Text::trim(authorTag) == "")) {
            ImGui::Text("Artist: %s", authorTag.c_str());
        }
        std::string albumTag = std::string(Mix_GetMusicAlbumTag(current_sound));
        if (!(Text::trim(albumTag) == "")) {
            ImGui::Text("Album: %s", albumTag.c_str());
        }
        std::string copyrightTag = std::string(Mix_GetMusicCopyrightTag(current_sound));
        if (!(Text::trim(copyrightTag) == "")) {
            ImGui::Text("Copyright: %s", copyrightTag.c_str());
        }

        int freq, channels;
        SDL_AudioFormat format;
        if (Mix_QuerySpec(&freq, &format, &channels)) {
            ImGui::Text("Sample Rate: %d kHz", freq / 1000);
            // If there are more than 2 channels, this will report as stereo
            // eventually i'll do something about this but im lazy :)
            ImGui::Text("%s", channels >= 2 ? "Stereo Audio" : "Mono Audio");
        }
    }
}

void PreviewWindow::RenderElfPreview() {
    auto elfFile = preview_state.contents.elfFile;
    ImGui::Text("Path: %s", preview_state.contents.path.c_str());
    ImGui::Text("ELF Class: %s", elfFile->GetElfClass().c_str());
    if (auto elfHeader = elfFile->GetElf64Header()) {
        ImGui::Text("Type: %s", elfFile->GetElfType(elfHeader->e_type).c_str());
        ImGui::Text("OS/ABI: %s", elfFile->GetElfOSABI(elfHeader->e_ident.os_abi).c_str());

        #ifdef WIN32
        ImGui::Text("Entry: 0x%llx", elfHeader->e_entry);
        #else
        ImGui::Text("Entry: 0x%lx", elfHeader->e_entry);
        #endif
    } else if (auto elfHeader = elfFile->GetElf32Header()) {
        ImGui::Text("Type: %s", elfFile->GetElfType(elfHeader->e_type).c_str());
        ImGui::Text("OS/ABI: %s", elfFile->GetElfOSABI(elfHeader->e_ident.os_abi).c_str());
        ImGui::Text("Entry: 0x%x", elfHeader->e_entry);
    } else {
        ImGui::Text("Failed to read ELF header!");
    }
}

void PreviewWindow::RenderTextViewer(ImGuiIO &io) {
    // TODO: handle different encodings
    // maybe add a dropdown for the user to select the encoding?
    if (io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_S) && text_editor__unsaved_changes) {
        std::string text = editor.GetText();
        if (!text.empty() && text.back() == '\n') {
            text.pop_back();
        }
        std::ofstream file(preview_state.contents.path, std::ios::binary);
        file << text;
        file.close();
        ReloadRootNode(rootNode);
        text_editor__unsaved_changes = false;
    }
    if (editor.IsTextChanged()) {
        text_editor__unsaved_changes = true;
    }

    editor.Render("TextEditor", {0, 0}, false);
}
