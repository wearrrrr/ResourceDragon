#include "Render.h"
#include <Clipboard.h>
#include <DirectoryNode.h>
#include <PreviewWindow.h>
#include <Themes.h>
#include <UIError.h>
#include "SDL3/SDL_video.h"
#include "imgui.h"
#include "state.h"
#include <util/Logger.h>

#include <filesystem>
namespace fs = std::filesystem;

#ifdef DEBUG
#include <cmath>
#define FPS_OVERLAY_FLAGS ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs
#endif

#define DIRECTORY_LIST_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
#define FILE_PREVIEW_FLAGS   DIRECTORY_LIST_FLAGS | ImGuiWindowFlags_HorizontalScrollbar

void RenderFBContextMenu(ImGuiIO *io) {
    if (ImGui::BeginPopupContextWindow("FBContextMenu")) {
        if (ImGui::BeginMenu("Copy..")) {
            if (ImGui::MenuItem("Name")) {
                ImGui::SetClipboardText(fb__selectedItem->FileName.c_str());
            }
            if (ImGui::MenuItem("Location")) {
                ImGui::SetClipboardText(fb__selectedItem->FullPath.c_str());
            }
            if (fb__selectedItem) {
                if (!fb__selectedItem->IsDirectory) {
                    if (ImGui::MenuItem("File")) {
                        if (rootNode->IsVirtualRoot) {
                            VirtualArc::ExtractEntry("/tmp/rd/", selected_entry, "/tmp/rd/" + selected_entry->name);
                            Clipboard::CopyFilePathToClipboard("/tmp/rd/" + selected_entry->name);
                        } else {
                            Clipboard::CopyFilePathToClipboard(fb__selectedItem->FullPath);
                        }
                    }
                }
            }
            ImGui::EndMenu();
        }
        if (rootNode->IsVirtualRoot) {
            if (ImGui::MenuItem("Extract File")) VirtualArc::ExtractEntry();
            if (ImGui::MenuItem("Extract All")) VirtualArc::ExtractAll();
        }
        if (ImGui::MenuItem("Reload")) ReloadRootNode(rootNode);
        if (ImGui::MenuItem("Delete")) openDelPopup = true;

        ImGui::EndPopup();
    }

    ImGui::SetNextWindowSize({600, 175});
    ImGui::SetNextWindowPos({io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.5f}, ImGuiCond_None, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        if (fb__selectedItem) {
            ImGui::TextWrapped("Are you sure you'd like to delete %s?", fb__selectedItem->FileName.size() > 0 ? fb__selectedItem->FileName.c_str() : "<ITEM>");
            ImGui::Text("This cannot be undone!");
            if (ImGui::Button("Confirm", {100, 0})) {
                fs::remove_all(fb__selectedItem->FullPath);
                if (fb__selectedItem->FullPath == preview_state.contents.path) {
                    DirectoryNode::UnloadSelectedFile();
                }
                ReloadRootNode(rootNode);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", {100, 0})) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void PreviewContextMenu() {
    if (ImGui::BeginPopupContextItem("PreviewItemContextMenu")) {
        if (ImGui::MenuItem("Copy to Clipboard")) {
            if (preview_state.contents.size > 0) {
                if (preview_state.contents.type == ContentType::IMAGE) {
                    Clipboard::CopyBufferToClipboard(preview_state.contents.data, preview_state.contents.size, preview_state.contents.fileName);
                } else {
                    Clipboard::CopyFilePathToClipboard(preview_state.contents.path);
                }
            }
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
        ImGui::TextWrapped("%s", ui_error.title.c_str());
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


SDL_Window* window;

bool running = true;

bool GUI::InitRendering() {
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
    if (mode) {
        if (mode->w < 1600 || mode->h < 900) {
            window_flags |= SDL_WINDOW_MAXIMIZED;
        }
    }

    window = SDL_CreateWindow("ResourceDragon", 1600, 900, window_flags);

    if (!window)
    {
        Logger::error("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return false;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        Logger::error("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return false;
    }

    ImGui::CreateContext();

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetSwapInterval(1);

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_ShowWindow(window);

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImFontGlyphRangesBuilder range;
    ImVector<ImWchar> gr;

    range.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    range.AddRanges(io.Fonts->GetGlyphRangesKorean());
    range.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
    // These characters aren't in any of the above glyph ranges, but are common in CJK text.
    range.AddChar(0x203B);
    range.AddChar(0x25A0);
    range.AddChar(0x25CB);

    range.BuildRanges(&gr);

    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.GlyphMinAdvanceX = 18.0f;
    const constexpr ImWchar icon_ranges[] = { 0xe800, 0xe809 };

#ifdef EMSCRIPTEN
    #define FONT_PATH_BASE fs::path("/fonts")
#else
    #define FONT_PATH_BASE fs::path("fonts")
#endif

    auto font_path = FONT_PATH_BASE / "NotoSansCJKjp-Medium.otf";
    auto icon_font_path = FONT_PATH_BASE / "icons.ttf";

    if (fs::exists(font_path)) {
        auto noto = io.Fonts->AddFontFromFileTTF(font_path.string().c_str(), 24, nullptr, gr.Data);
        if (!noto) {
            Logger::warn("Failed to load main font!");
        }
    }
    if (fs::exists(icon_font_path)) {
        auto icons = io.Fonts->AddFontFromFileTTF(icon_font_path.string().c_str(), 18, &iconConfig, icon_ranges);
        if (!icons) {
            Logger::warn("Failed to load icons! This will cause some things to not render properly.");
        }
    }
    io.Fonts->Build();

    ThemeManager::SetTheme(Theme::BessDark);

    return true;
}

void GUI::StartRenderLoop(const char *path) {

    auto canonical_path = fs::canonical(path).string();

    SetFilePath(canonical_path);
    rootNode = DirectoryNode::CreateTreeFromPath(canonical_path);
    ImGuiIO &io = ImGui::GetIO();
    const constexpr float splitterWidth = 10.0f;
    const constexpr float minPanelSize = 400.0f;
    bool resizing = false;

    const std::string preview_win_label = "Preview";

    Logger::print_stacktrace("Test error with stacktrace..");

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)) {
                running = false;
            }

            if (event.type == SDL_EVENT_DROP_FILE) {
                const char *dropped_filedir = event.drop.data;
                // Preview the dropped file
                auto dropped_path = fs::path(dropped_filedir);
                auto dropped_path_dir = dropped_path.parent_path();
                if (fs::exists(dropped_path)) {
                    if (fs::is_directory(dropped_path)) {
                        dropped_path_dir = dropped_path;
                    }
                    DirectoryNode::Node *newNode = DirectoryNode::CreateTreeFromPath(dropped_path_dir.string());
                    rootNode = newNode;
                    DirectoryNode::Node *itemNode = new DirectoryNode::Node {
                        .FullPath = dropped_path.string(),
                        .FileName = dropped_path.filename().string(),
                        .Parent = rootNode,
                        .IsDirectory = fs::is_directory(dropped_path),
                        .IsVirtualRoot = false,
                    };
                    HandleFileClick(itemNode);
                    SetFilePath(dropped_path.string());
                } else {
                    Logger::error("Failed to open file: %s", dropped_filedir);
                }
            }

            if (event.key.key == SDLK_F5) {
                ReloadRootNode(rootNode);
            }
            if ((event.key.mod & SDL_KMOD_CTRL) && event.key.key == SDLK_D) {
                DirectoryNode::UnloadSelectedFile();
            }
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();

        const ImVec2 window_size = io.DisplaySize;
        const ImVec2 mouse_pos = io.MousePos;

        static float left_pan_width = (window_size.x / 2.0f) - splitterWidth;
        static float right_pan_width = window_size.x - left_pan_width - splitterWidth;

        ImGui::NewFrame();

        ImGui::SetNextWindowSize({left_pan_width, window_size.y}, ImGuiCond_Always);
        ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
        if (ImGui::Begin("Directory Tree", NULL, DIRECTORY_LIST_FLAGS)) {
            RenderFBContextMenu(&io);
            RenderErrorPopup(&io);
            if (ui_error.show) {
                ImGui::OpenPopup("Error");
                ui_error.show = false;
            }
            if (rootNode) DirectoryNode::Setup(rootNode);
        }

        if (openDelPopup) {
            ImGui::OpenPopup("Delete Confirmation");
            openDelPopup = false;
        }

        ImGui::End();

        const bool hovered = (mouse_pos.x >= left_pan_width && mouse_pos.x <= left_pan_width + splitterWidth);

        if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) resizing = true;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) resizing = false;

        if (resizing) {
            left_pan_width += io.MouseDelta.x;
        }

        left_pan_width = std::max(minPanelSize, std::min(left_pan_width, window_size.x - minPanelSize - splitterWidth));
        right_pan_width = window_size.x - left_pan_width - splitterWidth;

        ImGui::SetNextWindowSize({right_pan_width, window_size.y});
        ImGui::SetNextWindowPos({left_pan_width + splitterWidth, 0});
        std::string preview_win_title = preview_win_label;
        if (text_editor__unsaved_changes) preview_win_title += " *";
        else preview_win_title += " ";
        preview_win_title += "###Preview";
        if(ImGui::Begin(preview_win_title.c_str(), NULL, FILE_PREVIEW_FLAGS)) {
            PreviewContextMenu();
            if (preview_state.contents.size > 0) {
                PreviewWindow::RenderPreviewFor(preview_state.contents.type);

                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup("PreviewItemContextMenu");
                }
            } else {
                ImGui::Text("No file selected.");
            }
        }
        ImGui::End();

        #ifdef DEBUG
        constexpr float DISTANCE = 8.0f;
        const ImVec2 window_pos = {
            DISTANCE,
            window_size.y - DISTANCE
        };
        const constexpr ImVec2 window_pos_pivot = {0.0f, 1.0f};

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.55f);

        if (ImGui::Begin("FPS Overlay", nullptr, FPS_OVERLAY_FLAGS)) {
            ImGui::Text("FPS: %.*f", 0, std::ceil(io.Framerate));
        }
        ImGui::End();


        #endif

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            quitDialog = !quitDialog;
            if (quitDialog) {
                ImGui::OpenPopup("Quit Confirmation");
            }
        }

        ImGui::Render();

        glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        static const constexpr ImVec4 clear_color = {0.23f, 0.23f, 0.23f, 1.00f};
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    MIX_DestroyAudio(preview_state.audio.music);

    SDL_DestroyWindow(window);
    SDL_RemoveTimer(preview_state.audio.update_timer);
    SDL_Quit();
}
