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

// TODO: This should probably be split out into two separate context menus, one firing when an item is hovered, and one when nothing is hovered.
void RenderContextMenu(ImGuiIO *io) {
    if (ImGui::BeginPopupContextWindow("ContextMenu")) {
        if (ImGui::BeginMenu("Copy..")) {
            if (ImGui::MenuItem("Name")) {
                ImGui::SetClipboardText(selectedItem.FileName.c_str());
            }
            if (ImGui::MenuItem("Location")) {
                ImGui::SetClipboardText(selectedItem.FullPath.c_str());
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Reload")) ReloadRootNode(rootNode);
        if (ImGui::MenuItem("Delete")) openDelPopup = true;
        ImGui::EndPopup();
    }


    ImGui::SetNextWindowSize({600, 175});
    ImGui::SetNextWindowPos({io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.5f}, ImGuiCond_None, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("Delete Confirmation", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Are you sure you'd like to delete %s?", selectedItem.FileName.length() > 0 ? selectedItem.FileName.c_str() : "<ITEM>");
        ImGui::Text("This cannot be undone!");
        if (ImGui::Button("Confirm", {100, 0})) {
            fs::remove_all(selectedItem.FullPath);
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

int main(int argc, char* argv[]) {
    extractor_manager.registerFormat(std::make_unique<HSPArchive>());

    std::string path;
    if (argc < 2) {
        path = ".";
    } else {
        path = argv[1];
    }

    rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(path));

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

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

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                running = false;

            // Check for f5 and reload tree node
            if (event.key.key == SDLK_F5) {
                ReloadRootNode(rootNode);
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
            DisplayDirectoryNode(rootNode, rootNode, true);
        }
        RenderContextMenu(&io);

        if (openDelPopup == true) {
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
        if(ImGui::Begin("Preview", NULL, FILE_PREVIEW_FLAGS)) {
            std::string ext = preview_state.contents.ext;
            if (preview_state.contents.size > 0) {
                if (Image::IsImageExtension(ext)) {
                    PWinStateTexture *texture = &preview_state.texture;
                    ImVec2 image_size = ImVec2(texture->size.x, texture->size.y);
                    if (texture->id) {
                        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - image_size.x) * 0.5f, 50));
                        ImGui::Image(texture->id, image_size);
                    } else {
                        ImGui::Text("Failed to load image!");
                    }
                } else if (Image::IsGif(ext)) {
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
                } else {
                    // TODO: handle different potential encodings
                    // maybe using a dropdown for the user to select the encoding.
                    ImGui::TextUnformatted(preview_state.contents.data, (preview_state.contents.data + preview_state.contents.size));
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

    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}