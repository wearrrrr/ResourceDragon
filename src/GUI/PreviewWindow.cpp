#include <PreviewWindow.h>
#include <Audio.h>
#include <DirectoryNode.h>
#include <ImVec2Util.h>
#include <SDL3/SDL_audio.h>
#include <util/Text.h>
#include <imgui.h>
#include "SDL3/SDL_properties.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "state.h"
#include "icons.h"

bool PlaybackScrubber(const char *id, float *progress, float width, bool interactive = true) {
    ImGui::PushID(id);

    auto height = 16.0f;

    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    const ImVec2 size = {width, height};

    ImGui::InvisibleButton("##scrubber", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    if (hovered && ImGui::IsMouseDown(0) && interactive) {
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

void PreviewWindow::RenderImagePreview() {
    PWinStateTexture *texture = &preview_state.texture;
    ImGui::Text("Zoom: %.2fx", image_preview.zoom);
    ImVec2 region_size = ImGui::GetContentRegionAvail();
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    ImGui::BeginChild("ImageRegion", region_size, false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

    if (ImGui::IsWindowHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

    // zooming
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
        const float prev_zoom = image_preview.zoom;
        int scroll_steps = (int)std::round(ImGui::GetIO().MouseWheel);
        image_preview.zoom = std::clamp(image_preview.zoom + scroll_steps * 0.1f, 0.1f, 5.0f);

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const ImVec2 rel = mouse - cursor_pos - image_preview.pan;
        image_preview.pan -= rel * (image_preview.zoom / prev_zoom - 1.0f);
    }

    // dragging
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        image_preview.pan += delta;
    }

    if (texture->id) {
        ImVec2 image_size = ImVec2(*texture->size.x * image_preview.zoom, *texture->size.y * image_preview.zoom);
        ImVec2 draw_pos = Floor(cursor_pos + image_preview.pan);
        ImVec2 draw_end = draw_pos + Floor(image_size);
        ImGui::GetWindowDrawList()->AddImage(texture->id, draw_pos, draw_end);
    } else {
        ImGui::Text("Failed to load image!");
    }
    ImGui::EndChild();
}

void PreviewWindow::RenderGifPreview() {
    PWinStateTexture *texture = &preview_state.texture;
    GifAnimation &anim = texture->anim;
    ImVec2 image_size = ImVec2(anim.width, anim.height);
    ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - image_size.x) * 0.5f, 50));
    ImGui::Image(Image::GetGifFrame(anim, &texture->frame), image_size);

    u32 now = SDL_GetTicks();
    u32 frame_delay = anim.delays[texture->frame];
    if (now - texture->last_frame_time >= frame_delay) {
        texture->frame = (texture->frame + 1) % anim.frame_count;
        texture->last_frame_time = now;
    }

    u32 elapsed = 0;
    for (int i = 0; i < texture->frame; ++i)
        elapsed += anim.delays[i];

    elapsed += (now - texture->last_frame_time);

    float progress = static_cast<float>(elapsed) / anim.total_duration_ms;

    float total_duration_ms = anim.total_duration_ms;
    float time_elapsed_ms = progress * total_duration_ms;
    int minutes = time_elapsed_ms / 60000.0f;
    float seconds = fmod(time_elapsed_ms / 1000.0f, 60.0f);

    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - image_size.x) * 0.5f);
    ImGui::Text("Time %02d:%05.2f", minutes, seconds);
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
    PlaybackScrubber("GifScrubber", &progress, 300.0f, false);
}



void SelectableCopyableText(const std::string& text) {
    static std::unordered_map<std::string, double> copiedTimestamps;
    constexpr double feedbackDuration = 1.0;

    ImGui::Selectable(text.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        ImGui::SetClipboardText(text.c_str());
        copiedTimestamps[text] = ImGui::GetTime();
    }

    if (ImGui::IsItemHovered()) {
        auto it = copiedTimestamps.find(text);
        if (it != copiedTimestamps.end() && (ImGui::GetTime() - it->second) < feedbackDuration) {
            ImGui::SetTooltip("Copied!");
        } else {
            ImGui::SetTooltip("Double-click to copy");
        }
    }

    if (copiedTimestamps.count(text) && ImGui::GetTime() - copiedTimestamps[text] >= feedbackDuration) {
        copiedTimestamps.erase(text);
    }
}

void PreviewWindow::RenderAudioPlayer() {
    if (preview_state.audio.music) {
        ImGui::Text("Playing: %s", preview_state.contents.path.c_str());
        TimeInfo time = preview_state.audio.time;
        if (preview_state.audio.playing) {
            if (ImGui::Button(PAUSE_ICON, {40, 0})) {
                MIX_PauseTrack(preview_state.audio.track);
                preview_state.audio.playing = false;
            }
        } else {
            if (ImGui::Button(PLAY_ICON, {40, 0})) {
                MIX_ResumeTrack(preview_state.audio.track);
                preview_state.audio.playing = true;
            }
        }
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (ImGui::Button(RW_ICON, {40, 0})) {
            auto track = preview_state.audio.track;
            Sint64 curr_pos = MIX_GetTrackPlaybackPosition(track);
            if (MIX_TrackFramesToMS(track, curr_pos) < 5000) {
                MIX_SetTrackPlaybackPosition(track, 0);
            } else {
                curr_pos -= MIX_TrackMSToFrames(track, 5000);
                MIX_SetTrackPlaybackPosition(track, curr_pos);
            }
        }
        ImGui::SameLine();
        ImGui::Text("%02d:%02d / %02d:%02d",
            time.current_time_min,
            time.current_time_sec,
            time.total_time_min,
            time.total_time_sec
        );
        ImGui::SameLine();
        const double current_pos = MIX_FramesToMS(preview_state.audio.spec.freq, MIX_GetTrackPlaybackPosition(preview_state.audio.track));
        int total_time = MIX_FramesToMS(preview_state.audio.spec.freq, MIX_GetAudioDuration(preview_state.audio.music));
        if (total_time > 0.0) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

            const double visual_pos = std::max(current_pos, 0.0);
            float scrubberProgress = visual_pos / total_time;
            scrubberProgress = std::clamp(scrubberProgress, 0.0f, 1.0f);

            bool isDragging = PlaybackScrubber("AudioScrubber", &scrubberProgress, (ImGui::GetWindowWidth() / 2.0f));

            if (isDragging) {
                if (!preview_state.audio.scrubberDragging)
                    MIX_PauseTrack(preview_state.audio.track);

                int new_pos_ms = scrubberProgress * total_time;
                timeToSetOnRelease = new_pos_ms;

                int new_pos_sec = new_pos_ms / 1000.0f;
                preview_state.audio.time.current_time_min = new_pos_sec / 60;
                preview_state.audio.time.current_time_sec = new_pos_sec % 60;

                preview_state.audio.scrubberDragging = true;
            }

            if (preview_state.audio.scrubberDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                MIX_SetTrackPlaybackPosition(preview_state.audio.track, MIX_MSToFrames(preview_state.audio.spec.freq, timeToSetOnRelease));
                MIX_ResumeTrack(preview_state.audio.track);
                preview_state.audio.scrubberDragging = false;
            }
        }
        ImGui::SameLine(0.0f, 16.0f);
        if (ImGui::Button(FF_ICON, {40, 0})) {
            auto track = preview_state.audio.track;
            Sint64 curr_pos = MIX_GetTrackPlaybackPosition(track);
            curr_pos += MIX_TrackMSToFrames(track, 5000);
            MIX_SetTrackPlaybackPosition(track, curr_pos);
        }
        ImGui::EndGroup();

        ImGui::SameLine();
        bool looping = preview_state.audio.shouldLoop;

        if (looping) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(117, 255, 154, 255));
        if (ImGui::Button(LOOP_ICON, {40, 0})) {
            preview_state.audio.shouldLoop = !preview_state.audio.shouldLoop;
        }
        if (looping) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button(STOP_ICON, {40, 0})) {
            MIX_DestroyTrack(preview_state.audio.track);
            DirectoryNode::UnloadSelectedFile();
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
            float sdlVolume = preview_state.audio.volumePercent / 100.0f;
            MIX_SetTrackGain(preview_state.audio.track, sdlVolume);
        }

        auto properties = MIX_GetAudioProperties(preview_state.audio.music);

        std::string titleTag = std::string(SDL_GetStringProperty(properties, MIX_PROP_METADATA_TITLE_STRING, "Unknown"));
        if (!Text::trim(titleTag).empty())
            SelectableCopyableText("Title: " + titleTag);

        std::string authorTag = std::string(SDL_GetStringProperty(properties, MIX_PROP_METADATA_ARTIST_STRING, "Unknown"));
        if (!Text::trim(authorTag).empty())
            SelectableCopyableText("Artist: " + authorTag);

        std::string albumTag = std::string(SDL_GetStringProperty(properties, MIX_PROP_METADATA_ALBUM_STRING, "Unknown"));
        if (!Text::trim(albumTag).empty())
            SelectableCopyableText("Album: " + albumTag);

        std::string copyrightTag = std::string(SDL_GetStringProperty(properties, MIX_PROP_METADATA_COPYRIGHT_STRING, "Unknown"));
        if (!Text::trim(copyrightTag).empty())
            SelectableCopyableText("Copyright: " + copyrightTag);

        SDL_AudioSpec spec = {
            .format = SDL_AUDIO_S16,
            .channels = 2,
            .freq = 44100,
        };
        if (MIX_GetAudioFormat(preview_state.audio.music, &spec)) {
            ImGui::Text("Sample Rate: %d kHz", spec.freq / 1000);
            // If there are more than 2 channels, this will report as stereo
            // eventually i'll do something about this but im lazy :)
            ImGui::Text("%s", spec.channels >= 2 ? "Stereo Audio" : "Mono Audio");
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

        #if defined(WIN32) || defined(EMSCRIPTEN)
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

static const char *encodings[] = {"UTF-8", "UTF-16", "Shift-JIS"};

void PreviewWindow::RenderTextViewer(ImGuiIO &io) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Content Encoding");
    ImGui::SameLine();
    int *encoding = (int*)&preview_state.contents.encoding;
    ImGui::Combo("##EncodingCombo", encoding, encodings, SIZEOF_ARRAY(encodings));
    ImGui::SameLine();
    if (ImGui::Button("Hex View")) {
        preview_state.contents.type = HEX;
    }

    if (currentEncoding != encodings[preview_state.contents.encoding]) {
        TextConverter::SetCurrentEncoding(encodings[preview_state.contents.encoding]);
        auto text = std::string((char*)preview_state.contents.data, preview_state.contents.size);
        editor.SetText(TextConverter::convert_to_utf8(text));
    }

    if (io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_S) && text_editor__unsaved_changes) {
        std::string text = editor.GetText();
        // This addresses some weird behavior where every save would add a \n, even if there was already one at the file
        // this might be a linux-ism?
        if (!text.empty() && text.back() == '\n') {
            text.pop_back();
        }
        FILE *file = fopen(preview_state.contents.path.c_str(), "w");
        fwrite(text.c_str(), text.size(), sizeof(u8), file);
        fclose(file);
        ReloadRootNode(rootNode);
        text_editor__unsaved_changes = false;
    }
    if (editor.IsTextChanged()) {
        text_editor__unsaved_changes = true;
    }

    editor.Render("TextEditor", {0, 0}, false);
}

void PreviewWindow::RenderHexEditor(ImGuiIO &io) {
    if (ImGui::Button("Text View")) {
        preview_state.contents.type = ContentType::TEXT;
    }
    auto pos = font_registry.find("MonoFont");
    ImFont *font;
    if (pos == font_registry.end()) {
        font = io.FontDefault;
    } else {
        font = pos->second;
    }
    ImGui::PushFont(font);
    hex_editor.DrawContents(preview_state.contents.data, preview_state.contents.size);
    ImGui::PopFont();
}
