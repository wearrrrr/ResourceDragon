#include "common.h"

#define DEBUG

#ifdef DEBUG
#define FPS_OVERLAY_FLAGS ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs
#endif

#define BACKGROUND_WIN_FLAGS ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs
#define DIRECTORY_TREE_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
#define FILE_PREVIEW_FLAGS   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_HorizontalScrollbar

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
                ImGui::SetClipboardText(selectedItem.FileName.c_str());
            }
            if (ImGui::MenuItem("Location")) {
                ImGui::SetClipboardText(selectedItem.FullPath.c_str());
            }
            if (!selectedItem.IsDirectory) {
                if (ImGui::MenuItem("File")) {
                    Clipboard::CopyFilePathToClipboard(selectedItem.FullPath);
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
        ImGui::TextWrapped("Are you sure you'd like to delete %s?", selectedItem.FileName.length() > 0 ? selectedItem.FileName.c_str() : "<ITEM>");
        ImGui::Text("This cannot be undone!");
        if (ImGui::Button("Confirm", {100, 0})) {
            fs::remove_all(selectedItem.FullPath);
            // Check if the file we are trying to delete is the currently selected file
            if (selectedItem.FullPath == preview_state.contents.path) {
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

int main(int argc, char* argv[]) {
    extractor_manager.registerFormat(std::make_unique<HSPArchive>());
    extractor_manager.registerFormat(std::make_unique<XP3Format>());

    std::string path;
    if (argc < 2) {
        path = ".";
    } else {
        path = argv[1];
    }

    rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(path));

    #ifdef linux
    // Add inotify watch
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
    // Add U+203B (Reference Mark) as a valid character
    range.AddChar(0x203B);
    range.BuildRanges(&gr);

    #ifdef WIN32
    const char *font_path = "fonts\\NotoSansCJK-Medium.ttc";
    #else
    const char *font_path = "fonts/NotoSansCJK-Medium.ttc";
    #endif

    
    if (fs::exists(font_path)) {
        io.Fonts->AddFontFromFileTTF(font_path, 24, nullptr, gr.Data);
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
    static bool resizing = false;

    bool running = true;
    ImVec4 clear_color = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);

    std::string preview_win_label = "Preview";

    bool has_unsaved_changes = false;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)))
                running = false;

            if (event.type == SDL_EVENT_DROP_FILE) {
                const char* dropped_filedir = event.drop.data;
                // Preview the dropped file
                fs::path dropped_path = fs::path(dropped_filedir);
                fs::path dropped_path_dir = dropped_path.parent_path();
                Logger::log("%s", dropped_path_dir.c_str());
                if (fs::exists(dropped_path)) {
                    rootNode = CreateDirectoryNodeTreeFromPath(dropped_path_dir);
                    selectedItem = CreateDirectoryNodeTreeFromPath(dropped_path);
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

        static float leftPanelWidth = (window_size.x / 2.0f) - splitterWidth;
        static float rightPanelWidth = window_size.x - leftPanelWidth - splitterWidth;

        
        ImGui::NewFrame();

        // ImGui::ShowDemoWindow();

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(window_size);
        ImGui::Begin("BackgroundRender", nullptr, BACKGROUND_WIN_FLAGS);

        ImGui::GetWindowDrawList()->AddRectFilled(
            {leftPanelWidth, 10},
            {leftPanelWidth + splitterWidth, window_size.y},
            IM_COL32(58, 58, 58, 255)
        );

        ImGui::End();

        ImGui::SetNextWindowSize({leftPanelWidth, window_size.y}, ImGuiCond_Always);
        ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
        if (ImGui::Begin("Directory Tree", NULL, DIRECTORY_TREE_FLAGS)) {
            RenderFBContextMenu(&io);
            RenderErrorPopup(&io);
            if (ui_error.show) {
                ImGui::OpenPopup("Error");
                ui_error.show = false;
            }
            DisplayDirectoryNode(rootNode, rootNode, true);
        }

        if (openDelPopup) {
            ImGui::OpenPopup("Delete Confirmation");
            openDelPopup = false;
        }

        ImGui::End();

        bool hovered = (mouse_pos.x >= leftPanelWidth && mouse_pos.x <= leftPanelWidth + splitterWidth);

        if (hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) resizing = true;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) resizing = false;

        if (resizing)
        {
            leftPanelWidth += io.MouseDelta.x;
        }

        leftPanelWidth = std::max(minPanelSize, std::min(leftPanelWidth, window_size.x - minPanelSize - splitterWidth));
        rightPanelWidth = window_size.x - leftPanelWidth - splitterWidth;

        ImGui::SetNextWindowSize({rightPanelWidth, window_size.y});
        ImGui::SetNextWindowPos({leftPanelWidth + splitterWidth, 0});
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
                        ImGui::SetCursorPos({(ImGui::GetWindowSize().x - texture->size.x) * 0.5f, 50});
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
                        // Time info breaks if the audio file is a midi file, pretty sure this is unfixable?
                        ImGui::Text("Playing: %s", preview_state.contents.path.c_str());
                        if (!curr_sound_is_midi) {
                            TimeInfo time = preview_state.audio.time;
                            // Display progress / total time
                            ImGui::Text("Current Time: %02d:%02d / %02d:%02d", 
                                time.current_time_min, 
                                time.current_time_sec,
                                time.total_time_min, 
                                time.total_time_sec
                            );
                        }
                        if (!curr_sound_is_midi) {
                            if (ImGui::Button("RW", {40, 0})) {
                                double new_pos = Mix_GetMusicPosition(current_sound) - 5.0;
                                if (new_pos > 0) {
                                    Mix_SetMusicPosition(new_pos);
                                    preview_state.audio.time.current_time_min = (int)new_pos / 60;
                                    preview_state.audio.time.current_time_sec = (int)new_pos % 60;
                                } else {
                                    // Prevent going negative
                                    Mix_SetMusicPosition(0);
                                    preview_state.audio.time.current_time_min = 0;
                                    preview_state.audio.time.current_time_sec = 0;
                                }
                            }
                            ImGui::SameLine();
                        }
                        if (preview_state.audio.playing) {
                            if (ImGui::Button("Pause", {75, 0})) {
                                Mix_PauseMusic();
                                preview_state.audio.playing = false;
                            }
                        } else {
                            if (ImGui::Button("Play", {75, 0})) {
                                Mix_ResumeMusic();
                                preview_state.audio.playing = true;
                                current_sound = Mix_PlayingMusic() ? current_sound : nullptr;
                            }
                        }
                        if (!curr_sound_is_midi) {
                            ImGui::SameLine();
                            // Fast forward 5 seconds
                            if (ImGui::Button("FF", {40, 0})) {
                                double new_pos = Mix_GetMusicPosition(current_sound) + 5.0;
                                if (new_pos > 0) {
                                    Mix_SetMusicPosition(new_pos);
                                    preview_state.audio.time.current_time_min = (int)new_pos / 60;
                                    preview_state.audio.time.current_time_sec = (int)new_pos % 60;
                                }
                            }
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Stop")) {
                            Mix_HaltMusic();
                            UnloadSelectedFile();
                            preview_state.audio.playing = false;
                            SDL_RemoveTimer(preview_state.audio.update_timer);
                            preview_state.audio.update_timer = 0;

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

    #ifdef linux
    inotify_running = false;
    close(inotify_fd);
    #endif
    
    return 0;
}