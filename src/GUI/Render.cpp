#include "Render.h"
#include <Clipboard.h>
#include <DirectoryNode.h>
#include <PreviewWindow.h>
#include <Themes.h>
#include <UIError.h>
#include "SDL3/SDL_video.h"
#include "imgui.h"
#include "imgui/misc/freetype/imgui_freetype.h"
#include "imgui_internal.h"
#include "state.h"
#include <util/Logger.h>

#include <filesystem>
namespace fs = std::filesystem;

#define DIRECTORY_LIST_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
#define FILE_PREVIEW_FLAGS   DIRECTORY_LIST_FLAGS | ImGuiWindowFlags_HorizontalScrollbar

void RenderFBContextMenu(ImGuiIO *io) {
    if (ImGui::BeginPopupContextWindow("FBContextMenu")) {
        if (ImGui::BeginMenu("Open As...")) {
            // Open fb__selectedItem as whatever is selected.
            if (ImGui::MenuItem("Text")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::TEXT);
            }
            if (ImGui::MenuItem("Hex")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::HEX);
            }
            if (ImGui::MenuItem("Image")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::IMAGE);
            }
            if (ImGui::MenuItem("Audio")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::AUDIO);
            }
            ImGui::EndMenu();
        }
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


#ifdef EMSCRIPTEN
    #define FONT_PATH_BASE fs::path("/fonts")
#else
    #define FONT_PATH_BASE fs::path("fonts")
#endif

void LoadFont(ImGuiIO& io, fs::path font_path, const char *font_name, const ImVector<ImWchar>& ranges, ImFontConfig *cfg = nullptr) {
    if (cfg) {
        cfg->OversampleH = 2;
        cfg->OversampleV = 2;
    }
    if (fs::exists(font_path)) {
        auto font = io.Fonts->AddFontFromFileTTF(font_path.string().c_str(), 26, cfg, ranges.Data);
        if (!font) {
            Logger::warn("Failed to load main font!");
        }
        font_registry[font_name] = font;
    }
}

void ConfigureDockSpace(bool* p_open) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoNavFocus |
                                  ImGuiWindowFlags_NoBackground |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("DockSpaceHost", p_open, host_flags);
    ImGui::PopStyleVar(3);

    ImGuiID root_id = ImGui::GetID("RootDockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton;
    ImGui::DockSpace(root_id, ImVec2(0,0), dockspace_flags);

    static bool first_time = true;
    if (first_time) {
        first_time = false;

        ImGui::DockBuilderRemoveNode(root_id);
        ImGui::DockBuilderAddNode(root_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(root_id, viewport->Size);

        ImGuiID dock_left, dock_right;
        ImGui::DockBuilderSplitNode(root_id, ImGuiDir_Left, 0.5f, &dock_left, &dock_right);
        ImGui::DockBuilderGetNode(dock_left)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDocking;

        ImGui::DockBuilderDockWindow("Directory Tree", dock_left);
        ImGui::DockBuilderDockWindow("Preview", dock_right);

        ImGui::DockBuilderFinish(root_id);
    }

    ImGui::End();
}

SDL_Window* window;

bool running = true;

bool GUI::InitRendering() {
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    window = SDL_CreateWindow("ResourceDragon", 1600, 900, window_flags);

    if (!window)
    {
        Logger::error("Error: SDL_CreateWindow(): {}", SDL_GetError());
        return false;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        Logger::error("Error: SDL_GL_CreateContext(): {}", SDL_GetError());
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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImFontGlyphRangesBuilder range;
    ImVector<ImWchar> gr;

    const constexpr ImWchar custom_ranges[] = { 0x1, 0x1FFFF, 0 };;
    range.AddRanges(custom_ranges);

    range.BuildRanges(&gr);

    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.GlyphMinAdvanceX = 18.0f;
    const constexpr ImWchar icon_ranges[] = { 0xe800, 0xe809 };

    auto noto_font_path = FONT_PATH_BASE / "NotoSansCJK-Medium.otf";
#ifdef __linux__
    const char *linux_font_path = "/usr/share/fonts/noto-cjk/NotoSansCJK-Medium.ttc";
    if (fs::exists(linux_font_path)) {
        noto_font_path = fs::path(linux_font_path);
    };
    LoadFont(io, noto_font_path, "UIFont", gr);
#else
    LoadFont(io, noto_font_path, "UIFont", gr);
#endif

    auto emoji_font = "fonts/twemoji.ttf";

    if (fs::exists(emoji_font)) {
        ImFontConfig emojiConfig;
        emojiConfig.MergeMode = true;
        emojiConfig.GlyphMinAdvanceX = 18.0f;
        emojiConfig.GlyphOffset.y = 1.0f;
        emojiConfig.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LoadColor;

        io.Fonts->AddFontFromFileTTF(
            emoji_font,
            24.0f,
            &emojiConfig,
            gr.Data
        );
    }

    auto mono_font_path = FONT_PATH_BASE / "SpaceMono-Regular.ttf";
    auto icon_font_path = FONT_PATH_BASE / "icons.ttf";

    // For some reason, things break if I change the order in which these are loaded.. very cool.
    if (fs::exists(icon_font_path)) {
        auto icons = io.Fonts->AddFontFromFileTTF(icon_font_path.string().c_str(), 20, &iconConfig, icon_ranges);
        if (!icons) {
            Logger::warn("Failed to load icons! This will cause some things to not render properly.");
        }
    } else {
        Logger::warn("Failed to find icons.ttf, icons will not properly render...");
    }

    LoadFont(io, mono_font_path, "MonoFont", gr);



    io.Fonts->Build();

    theme_manager.LoadThemes();
    theme_manager.AdjustmentStyles();
    theme_manager.SetTheme(Theme::Dark);

    editor.SetColorizerEnable(false);
    editor.SetShowWhitespaces(false);

    return true;
}

void GUI::StartRenderLoop() {
    ImGuiIO &io = ImGui::GetIO();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)) {
                running = false;
            }

            if (event.type == SDL_EVENT_DROP_FILE) {
                const char *dropped_filedir = event.drop.data;
                auto dropped_path = fs::path(dropped_filedir);
                auto dropped_path_dir = dropped_path.parent_path();
                if (fs::exists(dropped_path)) {
                    if (fs::is_directory(dropped_path)) {
                        dropped_path_dir = dropped_path;
                    }
                    rootNode = DirectoryNode::CreateTreeFromPath(dropped_path_dir.string());
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
                    Logger::error("Failed to open file: {}", dropped_filedir);
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

        ImGui::NewFrame();

        DirectoryNode::ProcessPendingFileLoads();

        // ImGui::ShowDemoWindow(&running);

        ConfigureDockSpace(nullptr);

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

        int preview_flags;
        if (text_editor__unsaved_changes) preview_flags = FILE_PREVIEW_FLAGS | ImGuiWindowFlags_UnsavedDocument;
        else preview_flags = FILE_PREVIEW_FLAGS;

        if(ImGui::Begin("Preview", NULL, preview_flags)) {
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

        ImGui::SetNextWindowPos(
            ImVec2(window_size.x - 375.0f, window_size.y),
            ImGuiCond_Always,
            ImVec2(0.0f, 1.0f)
        );

        if (fb__loading_arc) {
            if (ImGui::Begin("Bottom Bar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
                #define MAX_LENGTH_WITH_TRUNCATE 25
                // annoying but necessary
                    char loading_text[MAX_LENGTH_WITH_TRUNCATE + sizeof("Loading: ...")];
                    sprintf(
                        loading_text,
                        fb__loading_file_name.length() <= MAX_LENGTH_WITH_TRUNCATE + 3
                            ? "Loading: %s"
                            : "Loading: %." MACRO_STR(MAX_LENGTH_WITH_TRUNCATE) "s..."
                        , fb__loading_file_name.c_str()
                    );

                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), {350.0f, 30.0f}, loading_text);
            }
            ImGui::End();
        }

        ImGui::Render();

        glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;
        ImVec4 clear_color = colors[ImGuiCol_WindowBg];

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
