#include <GUI/Audio.h>
#include <GUI/Theme/Themes.h>
#include <GUI/DirectoryNode.h>
#include <GUI/Clipboard.h>
#include <GUI/PreviewWindow.h>
#include <GUI/UIError.h>
#include <Scripting/ScriptManager.h>
#include <thread>
#include <filesystem>

#include "state.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <ArchiveFormats/HSP/hsp.h>
#include <ArchiveFormats/PFS/pfs.h>
#include <ArchiveFormats/NitroPlus/nitroplus.h>
#include <ArchiveFormats/SonicAdv/sonicadv.h>
#include <ArchiveFormats/Touhou/thdat.h>
#include <ArchiveFormats/XP3/xp3.h>
#include <ArchiveFormats/Zip/zip.h>

namespace fs = std::filesystem;

#define DEBUG

#ifdef DEBUG
#include <cmath>

#define FPS_OVERLAY_FLAGS ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs
#endif

#define BACKGROUND_WIN_FLAGS ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs
#define DIRECTORY_TREE_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
#define FILE_PREVIEW_FLAGS   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_HorizontalScrollbar

bool openDelPopup = false;
bool running = true;
bool quitDialog = false;

void RenderFBContextMenu(ImGuiIO *io) {
    if (ImGui::BeginPopupContextWindow("FBContextMenu")) {
        if (ImGui::BeginMenu("Copy..")) {
            if (ImGui::MenuItem("Name")) {
                ImGui::SetClipboardText(selectedItem->FileName.c_str());
            }
            if (ImGui::MenuItem("Location")) {
                ImGui::SetClipboardText(selectedItem->FullPath.c_str());
            }
            if (selectedItem) {
                if (!selectedItem->IsDirectory) {
                    if (ImGui::MenuItem("File")) {
                        if (rootNode->IsVirtualRoot) {
                            VirtualArc::ExtractEntry("/tmp/rd/", selected_entry, "/tmp/rd/image.png");
                            Clipboard::CopyFilePathToClipboard("/tmp/rd/image.png");
                        } else {
                            Clipboard::CopyFilePathToClipboard(selectedItem->FullPath);
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
        if (selectedItem) {
            ImGui::TextWrapped("Are you sure you'd like to delete %s?",
                selectedItem->FileName.size() > 0 ? selectedItem->FileName.c_str() : "<ITEM>");
            ImGui::Text("This cannot be undone!");
            if (ImGui::Button("Confirm", {100, 0})) {
                fs::remove_all(selectedItem->FullPath);
                if (selectedItem->FullPath == preview_state.contents.path) {
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

void RenderPreviewContextMenu() {
    if (ImGui::BeginPopupContextItem("PreviewItemContextMenu")) {
        if (ImGui::MenuItem("Copy to Clipboard")) {
            if (preview_state.contents.size > 0) {
                if (preview_state.content_type == PContentType::IMAGE) {
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

template <class T>
inline void RegisterFormat() {
    extractor_manager.RegisterFormat(std::make_unique<T>());
}

template <class T>
inline void RegisterFormat(T *arc_fmt) {
    extractor_manager.RegisterFormat(std::unique_ptr<T>(arc_fmt));
}

int main(int argc, char *argv[]) {
    RegisterFormat<HSPArchive>();
    RegisterFormat<PFSFormat>();
    RegisterFormat<NitroPlus::NPK>();
    RegisterFormat<NitroPlus::MPK>();
    RegisterFormat<SonicAdv::PAK>();
    RegisterFormat<THDAT>();
    RegisterFormat<XP3Format>();
    RegisterFormat<ZipFormat>();

    ScriptManager *scriptManager = new ScriptManager();

    std::thread scripting_thread([&]() {
        try {
            for (const auto &entry : fs::directory_iterator("scripts/")) {
                const fs::path entry_path = entry.path();
                if (entry_path.extension() == ".lua") {
                    scriptManager->LoadFile(entry_path.string());
                    RegisterFormat<LuaArchiveFormat>(scriptManager->Register());
                }
            }
        } catch (const fs::filesystem_error &e) {
            Logger::error("Failed to start scripting! Error: %s", e.what());
        }

    });
    scripting_thread.detach();

    #ifdef linux
    // Clear temp dir on startup, this invalidates a file copied to the clipboard from a previous run, but that's fine i guess.
    fs::remove_all("/tmp/rd/");
    #endif

    const char *path;
    if (argc < 2) {
        path = ".";
    } else {
        path = argv[1];
    }

    auto canonical_path = fs::canonical(path);

    SetFilePath(canonical_path);
    rootNode = DirectoryNode::CreateTreeFromPath(canonical_path);

    #ifdef linux
    #define INOTIFY_FLAGS IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE
    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        Logger::error("Error: inotify_init() failed");
        return -1;
    }
    inotify_wd = inotify_add_watch(inotify_fd, path, INOTIFY_FLAGS);
    if (inotify_wd < 0) {
        Logger::error("Error: inotify_add_watch() failed");
        close(inotify_fd);
        return -1;
    }
    bool inotify_running = true;
    std::thread inotify_thread([&]() {
        char buffer[1024];
        while (inotify_running) {
            int length = read(inotify_fd, buffer, sizeof(buffer));
            if (length < 0) {
                Logger::error("Error: read() failed from inotify_fd");
                break;
            }
            int i = 0;
            while (i < length) {
                inotify_event *event = (inotify_event*)&buffer[i];
                if (event->mask & (INOTIFY_FLAGS)) {
                    ReloadRootNode(rootNode);
                }
                i += EVENT_SIZE + event->len;
            }
        }
    });
    inotify_thread.detach();
    #endif

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        Logger::error("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    Audio::InitAudioSystem();

    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
    if (mode) {
        if (mode->w < 1600 || mode->h < 900) {
            window_flags |= SDL_WINDOW_MAXIMIZED;
        }
    }

    SDL_Window* window = SDL_CreateWindow("ResourceDragon", 1600, 900, window_flags);
    if (!window)
    {
        Logger::error("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        Logger::error("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImFontGlyphRangesBuilder range;
    ImVector<ImWchar> gr;

    range.AddRanges(io.Fonts->GetGlyphRangesDefault());
    range.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    range.AddRanges(io.Fonts->GetGlyphRangesKorean());
    range.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
    range.AddRanges(io.Fonts->GetGlyphRangesThai());
    range.AddRanges(io.Fonts->GetGlyphRangesVietnamese());
    range.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
    range.AddRanges(io.Fonts->GetGlyphRangesGreek());
    // These characters aren't in any of the above glyph ranges, but are common in CJK text.
    range.AddChar(0x203B);
    range.AddChar(0x25A0);
    range.AddChar(0x25CB);
    range.BuildRanges(&gr);

    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.GlyphMinAdvanceX = 18.0f;
    const ImWchar icon_ranges[] = { 0xe800, 0xe809 };

    // I sure do love operator overloading :clueless:
    auto font_path = fs::path("fonts") / "NotoSansCJKjp-Medium.woff2";
    auto icon_font_path = fs::path("fonts") / "icons.woff2";

    auto noto = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 24, nullptr, gr.Data);
    auto icons = io.Fonts->AddFontFromFileTTF(icon_font_path.c_str(), 18, &iconConfig, icon_ranges);
    if (!noto) {
        Logger::warn("Failed to load main font! Continuing with default...");
    }
    if (!icons) {
        Logger::warn("Failed to load icons! This will cause some things to not render properly.");
    }

    ThemeManager::SetTheme(Theme::BessDark);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetSwapInterval(1);

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init();

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_ShowWindow(window);

    const float splitterWidth = 10.0f;
    const float minPanelSize = 400.0f;
    bool resizing = false;

    const std::string preview_win_label = "Preview";

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) {
                running = false;
            }

            if (event.type == SDL_EVENT_DROP_FILE) {
                const char *dropped_filedir = event.drop.data;
                // Preview the dropped file
                auto dropped_path = fs::path(dropped_filedir);
                auto dropped_path_dir = dropped_path.parent_path();
                if (fs::exists(dropped_path)) {
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

        static bool cleared_font_input_data = false;
        if (!cleared_font_input_data) {
            io.Fonts->ClearInputData();
            io.Fonts->ClearTexData();
            cleared_font_input_data = true;
        }

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(window_size);
        ImGui::Begin("BackgroundRender", nullptr, BACKGROUND_WIN_FLAGS);

        constexpr ImU32 bgColor = IM_COL32(58, 58, 58, 255);

        ImGui::GetWindowDrawList()->AddRectFilled(
            {left_pan_width, 10},
            {left_pan_width + splitterWidth, window_size.y},
            bgColor
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
            DirectoryNode::Setup(rootNode);
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

        if (resizing)
        {
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
            RenderPreviewContextMenu();
            std::string ext = preview_state.contents.ext;
            PContentType content_type = preview_state.content_type;
            if (preview_state.contents.size > 0) {
                switch (content_type) {
                    case IMAGE:
                        PreviewWindow::RenderImagePreview();
                        break;
                    case GIF:
                        PreviewWindow::RenderGifPreview();
                        break;
                    case AUDIO:
                        PreviewWindow::RenderAudioPlayer();
                        break;
                    case ELF:
                        PreviewWindow::RenderElfPreview();
                        break;
                    default:
                        PreviewWindow::RenderTextViewer(io);
                        break;
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
        const constexpr float DISTANCE = 8.0f;
        const ImVec2 window_pos = {DISTANCE, window_size.y - DISTANCE};
        const constexpr ImVec2 window_pos_pivot = {0.0f, 1.0f};

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.55f);

        if (ImGui::Begin("FPS Overlay", nullptr, FPS_OVERLAY_FLAGS))
        {
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

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        static const constexpr ImVec4 clear_color = {0.23f, 0.23f, 0.23f, 1.00f};
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

    DirectoryNode::Unload(rootNode);

    #ifdef linux
    inotify_running = false;
    close(inotify_fd);
    #endif

    return 0;
}
