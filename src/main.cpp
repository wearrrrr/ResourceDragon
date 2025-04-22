#include <iostream>
#include "common.h"

#define DEBUG

#ifdef DEBUG
#define FPS_OVERLAY_FLAGS ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs
#endif

#define BACKGROUND_WIN_FLAGS ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs
#define DIRECTORY_TREE_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
#define FILE_PREVIEW_FLAGS   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_HorizontalScrollbar

#define PLAY_ICON "\ue800"
#define LOOP_ICON "\ue801"
#define STOP_ICON "\ue802"
#define PAUSE_ICON "\ue803"
#define FF_ICON "\ue804"
#define RW_ICON "\ue805"

#ifdef WIN32
std::string separator = "\\";
#else
std::string separator = "/";
#endif

bool openDelPopup = false;

void RenderFBContextMenu(ImGuiIO *io) {
    if (ImGui::BeginPopupContextWindow("FBContextMenu")) {
        if (ImGui::BeginMenu("Copy..")) {
            if (ImGui::MenuItem("Name")) {
                ImGui::SetClipboardText(selectedItem->FileName.c_str());
            }
            if (ImGui::MenuItem("Location")) {
                ImGui::SetClipboardText(selectedItem->FullPath.c_str());
            }
            // TODO: We need to be clearing selectedItem's pointer every frame if nothing is hovering, but for now this is here 
            // this prevents shit from breaking.
            if (selectedItem) {
                if (!selectedItem->IsDirectory) {
                    if (ImGui::MenuItem("File")) {
                        Clipboard::CopyFilePathToClipboard(selectedItem->FullPath);
                    }
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Reload")) ReloadRootNode(rootNode);
        if (ImGui::MenuItem("Delete")) openDelPopup = true;
        ImGui::EndPopup();
    }


    ImGui::SetNextWindowSize({600, 175});
    ImGui::SetNextWindowPos({io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.5f}, ImGuiCond_None, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextWrapped("Are you sure you'd like to delete %s?", selectedItem->FileName.size() > 0 ? selectedItem->FileName.c_str() : "<ITEM>");
        ImGui::Text("This cannot be undone!");
        if (ImGui::Button("Confirm", {100, 0})) {
            fs::remove_all(selectedItem->FullPath);
            // If this is false, someone deleted a file that isn't selected
            if (selectedItem->FullPath == preview_state.contents.path) {
                UnloadSelectedFile();
            }
            ReloadRootNode(rootNode);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", {100, 0})) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    };
}

void RenderPreviewContextMenu(ImGuiIO *io) {
    if (ImGui::BeginPopupContextItem("PreviewItemContextMenu")) {
        if (ImGui::MenuItem("Copy to Clipboard")) {
            Clipboard::CopyFilePathToClipboard(preview_state.contents.path);
        }
        if (ImGui::MenuItem("Save...")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RenderErrorPopup(ImGuiIO *io) {
    ImGui::SetNextWindowSize({600, 250});
    ImGui::SetNextWindowPos({io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.5f}, ImGuiCond_None, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextWrapped("%s", ui_error.message.c_str());
        ImGui::Text("Press escape to close.");
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ui_error.show = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Close", {100, 0})) {
            ui_error.show = false;
            ImGui::CloseCurrentPopup();
        } 
        ImGui::SameLine();
        if (ImGui::Button("Copy to Clipboard", {175, 0})) {
            ImGui::SetClipboardText(ui_error.message.c_str());
        }
        ImGui::EndPopup();
    }
}

bool PlaybackScrubber(const char *id, float *progress, float width, float height = 16.0f) {
    ImGui::PushID(id);

    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 size = {width, height};

    ImGui::InvisibleButton("##scrubber", size);
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

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

template <class T>
inline void RegisterFormat() {
    extractor_manager.registerFormat(std::make_unique<T>());
}

inline void RegisterFormats() {
    RegisterFormat<HSPArchive>();
    RegisterFormat<PFSFormat>();
    RegisterFormat<NitroPlus::MPK>();
    RegisterFormat<XP3Format>();
}

int main(int argc, char* argv[]) {
    RegisterFormats();

    std::string path;
    if (argc < 2) {
        path = ".";
    } else {
        path = argv[1];
    }

    rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(path).string());

    #ifdef linux
    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        std::cerr << "Error: inotify_init() failed" << std::endl;
        return -1;
    }
    inotify_wd = inotify_add_watch(inotify_fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    if (inotify_wd < 0) {
        std::cerr << "Error: inotify_add_watch() failed" << std::endl;
        close(inotify_fd);
        return -1;
    }
    bool inotify_running = true;
    std::thread inotify_thread([&]() {
        char buffer[1024];
        while (inotify_running) {
            int length = read(inotify_fd, buffer, sizeof(buffer));
            if (length < 0) {
                Logger::log("Error: read() failed from inotify_fd");
                break;
            }
            int i = 0;
            while (i < length) {
                inotify_event *event = (inotify_event*)&buffer[i];
                if (event->mask & (IN_MODIFY | IN_CREATE | IN_DELETE)) {
                    ReloadRootNode(rootNode);
                }
                i += EVENT_SIZE + event->len;
            }
        }
    });
    inotify_thread.detach();
    #endif

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    Audio::InitAudioSystem();

    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    // If display is smaller than 1600x900, maximize the window.
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
    if (mode->w < 1600 || mode->h < 900) {
        window_flags |= SDL_WINDOW_MAXIMIZED;
    }

    SDL_Window* window = SDL_CreateWindow("ResourceDragon", 1600, 900, window_flags);
    if (!window)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }
    
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImFontGlyphRangesBuilder range;
    ImVector<ImWchar> gr;
    
    range.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    range.AddRanges(io.Fonts->GetGlyphRangesKorean());
    // U+203B (Reference Mark), Used in a lot of CJK text, but isn't a part of ImGui's glyph range ;-;
    range.AddChar(0x203B);
    range.BuildRanges(&gr);

    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.GlyphMinAdvanceX = 18.0f;

    const ImWchar icon_ranges[] = { 0xe800, 0xe805, 0 };

    #ifdef WIN32
    const char *font_path = "fonts\\NotoSansCJK-Medium.ttc";
    const char *icon_font_path = "fonts\\player-icons.ttf";
    #else
    const char *font_path = "fonts/NotoSansCJK-Medium.ttc";
    const char *icon_font_path = "fonts/player-icons.ttf";
    #endif
    
    if (fs::exists(font_path)) {
        io.Fonts->AddFontFromFileTTF(font_path, 24, nullptr, gr.Data);
        io.Fonts->AddFontFromFileTTF(icon_font_path, 18, &iconConfig, icon_ranges);
    } else {
        Logger::warn("Failed to locate font file! Attempted to search: %s", font_path);
    }

    Theme::SetTheme("BessDark");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetSwapInterval(1);

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_ShowWindow(window);

    float splitterWidth = 10.0f;
    float minPanelSize = 400.0f;
    
    bool resizing = false;
    bool running = true;
    bool has_unsaved_changes = false;
    
    ImVec4 clear_color = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
    std::string preview_win_label = "Preview";

    float timeToSetOnRelease = 0.0f;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)))
                running = false;

            if (event.type == SDL_EVENT_DROP_FILE) {
                const char* dropped_filedir = event.drop.data;
                // Preview the dropped file
                auto dropped_path = fs::path(dropped_filedir);
                auto dropped_path_dir = dropped_path.parent_path();
                Logger::log("%s", dropped_path_dir.c_str());
                if (fs::exists(dropped_path)) {
                    rootNode = CreateDirectoryNodeTreeFromPath(dropped_path_dir.string());
                    selectedItem = CreateDirectoryNodeTreeFromPath(dropped_path.string());
                    HandleFileClick(selectedItem);
                } else {
                    Logger::error("Dropped file does not exist: %s", dropped_filedir);
                }
            }

            if (event.key.key == SDLK_F5) {
                ReloadRootNode(rootNode);
            }
            if ((event.key.mod & (SDL_KMOD_CTRL)) && event.key.key == SDLK_D) {
                UnloadSelectedFile();
            }
        }
        
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();

        ImVec2 window_size = io.DisplaySize;
        ImVec2 mouse_pos = io.MousePos;

        static float left_pan_width = (window_size.x / 2.0f) - splitterWidth;
        static float right_pan_width = window_size.x - left_pan_width - splitterWidth;

        
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(window_size);
        ImGui::Begin("BackgroundRender", nullptr, BACKGROUND_WIN_FLAGS);

        ImGui::GetWindowDrawList()->AddRectFilled(
            {left_pan_width, 10},
            {left_pan_width + splitterWidth, window_size.y},
            IM_COL32(58, 58, 58, 255)
        );

        ImGui::End();

        ImGui::SetNextWindowSize({left_pan_width, window_size.y}, ImGuiCond_Always);
        ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
        if (ImGui::Begin("Directory Tree", NULL, DIRECTORY_TREE_FLAGS)) {
            RenderFBContextMenu(&io);
            RenderErrorPopup(&io);
            if (ui_error.show) {
                ImGui::OpenPopup("Error");
                ui_error.show = false;
            }
            DisplayDirectoryNode(rootNode);
        }

        if (openDelPopup) {
            ImGui::OpenPopup("Delete Confirmation");
            openDelPopup = false;
        }

        ImGui::End();

        bool hovered = (mouse_pos.x >= left_pan_width && mouse_pos.x <= left_pan_width + splitterWidth);

        if (hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) resizing = true;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) resizing = false;

        if (resizing)
        {
            left_pan_width += io.MouseDelta.x;
        }

        left_pan_width = std::max(minPanelSize, std::min(left_pan_width, window_size.x - minPanelSize - splitterWidth));
        right_pan_width = window_size.x - left_pan_width - splitterWidth;

        ImGui::SetNextWindowSize({right_pan_width, window_size.y});
        ImGui::SetNextWindowPos({left_pan_width + splitterWidth, 0});
        std::string preview_win_title = preview_win_label;
        if (has_unsaved_changes) preview_win_title += " *";
        else preview_win_title += " ";
        preview_win_title += "###Preview";
        if(ImGui::Begin(preview_win_title.c_str(), NULL, FILE_PREVIEW_FLAGS)) {
            RenderPreviewContextMenu(&io);
            std::string ext = preview_state.contents.ext;
            std::string content_type = preview_state.content_type;
            if (preview_state.contents.size > 0) {
                if (content_type == "image") {
                    PWinStateTexture *texture = &preview_state.texture;
                    ImVec2 image_size = ImVec2(texture->size.x, texture->size.y);
                    if (texture->id) {
                        if (image_size.x < window_size.x) {
                            ImGui::SetCursorPos({(ImGui::GetWindowSize().x - texture->size.x) * 0.5f, 50});
                        }
                        ImGui::Image(texture->id, image_size);
                    } else {
                        ImGui::Text("Failed to load image!");
                    }
                } else if (content_type == "gif") {
                    PWinStateTexture *texture = &preview_state.texture;
                    GifAnimation &anim = texture->anim;
                    ImVec2 image_size = ImVec2(anim.width, anim.height);
                    ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - image_size.x) * 0.5f, 50));
                    ImGui::Image(Image::GetGifFrame(anim, &texture->frame), image_size);

                    uint32_t now = SDL_GetTicks();
                    int frame_delay = anim.delays[texture->frame];
                    if (now - texture->last_frame_time >= frame_delay) {
                        texture->frame = (texture->frame + 1) % anim.frame_count;
                        texture->last_frame_time = now;
                    }
                } else if (content_type == "audio") {
                    // Display audio controls
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
                                current_sound = Mix_PlayingMusic() ? current_sound : nullptr;
                            }
                        }
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        // Time info breaks if the audio file is a midi file, pretty sure this is unfixable?
                        if (!curr_sound_is_midi) {
                            if (ImGui::Button(RW_ICON, {40, 0})) {
                                double new_pos = Mix_GetMusicPosition(current_sound) - 5.0;
                                if (new_pos > 0) {
                                    Mix_SetMusicPosition(new_pos);
                                    if (!preview_state.audio.scrubberDragging) {
                                        preview_state.audio.time.current_time_min = (int)new_pos / 60;
                                        preview_state.audio.time.current_time_sec = (int)new_pos % 60;
                                    }
                                } else {
                                    // Prevent going negative
                                    Mix_SetMusicPosition(0);
                                    preview_state.audio.time.current_time_min = 0;
                                    preview_state.audio.time.current_time_sec = 0;
                                }
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
                            double current_pos = Mix_GetMusicPosition(current_sound);
                            double total_time = time.total_time_min * 60 + time.total_time_sec;
                            if (total_time > 0.0) {
                                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

                                float visual_pos = std::max(current_pos - 0.3, 0.0);
                                float scrubberProgress = visual_pos / total_time;
                                scrubberProgress = std::clamp(scrubberProgress, 0.0f, 1.0f);
                                bool isDragging = PlaybackScrubber("AudioScrubber", &scrubberProgress, (ImGui::GetWindowWidth() / 2.0f));
                            
                                if (isDragging) {
                                    if (!preview_state.audio.scrubberDragging) Mix_PauseMusic();
                            
                                    double new_pos = scrubberProgress * total_time;
                                    timeToSetOnRelease = new_pos;
                                    preview_state.audio.time.current_time_min = (int)new_pos / 60;
                                    preview_state.audio.time.current_time_sec = (int)new_pos % 60;
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
                                if (new_pos > 0) {
                                    Mix_SetMusicPosition(new_pos);
                                    preview_state.audio.time.current_time_min = (int)new_pos / 60;
                                    preview_state.audio.time.current_time_sec = (int)new_pos % 60;
                                }
                            }
                            ImGui::EndGroup();
                        }

                        ImGui::SameLine();
                        bool looping = preview_state.audio.shouldLoop;
                        if (looping) {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(117, 255, 154, 255)); 
                        }
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
                    }
                } else if (content_type == "elf") {
                    ImGui::Text("Path: %s", preview_state.contents.path.c_str());
                    ImGui::Text("ELF Class: %s", preview_state.contents.elfFile->GetElfClass().c_str());
                    if (auto elfHeader = preview_state.contents.elfFile->GetElf64Header()) {
                        ImGui::Text("Entry: 0x%lx", elfHeader->e_entry);
                    } else if (auto elfHeader = preview_state.contents.elfFile->GetElf32Header()) {
                        ImGui::Text("Entry: 0x%x", elfHeader->e_entry);
                    } else {
                        Logger::error("Failed to read ELF header!");
                    }
                } else {
                    // TODO: handle different potential encodings
                    // maybe using a dropdown for the user to select the encoding.
                    if (io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_S) && has_unsaved_changes) {
                        std::string text = editor.GetText();
                        if (!text.empty() && text.back() == '\n') {
                            text.pop_back();
                        }
                        std::ofstream file(preview_state.contents.path, std::ios::binary);
                        file << text;
                        file.close();
                        ReloadRootNode(rootNode);
                        has_unsaved_changes = false;
                    }
                    if (editor.IsTextChanged()) {
                        has_unsaved_changes = true;
                    }

                    editor.Render("TextEditor", {0, 0}, false);
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup("PreviewItemContextMenu");
                }
            } else {
                ImGui::Text("No file selected.");
            }
        }
        ImGui::End();

        #ifdef DEBUG
        const float DISTANCE = 8.0f;
        ImVec2 window_pos = ImVec2(DISTANCE, window_size.y - DISTANCE);
        ImVec2 window_pos_pivot = ImVec2(0.0f, 1.0f);
    
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.55f);

        if (ImGui::Begin("FPS Overlay", nullptr, FPS_OVERLAY_FLAGS)) 
        {
            ImGui::Text("FPS: %d", (int)std::ceil(io.Framerate));
        }
        ImGui::End();
        #endif


        ImGui::Render();

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    
    SDL_DestroyWindow(window);
    SDL_RemoveTimer(preview_state.audio.update_timer);
    SDL_Quit();

    DeleteDirectoryNodeTree(rootNode);

    #ifdef linux
    inotify_running = false;
    close(inotify_fd);
    #endif
    
    return 0;
}
