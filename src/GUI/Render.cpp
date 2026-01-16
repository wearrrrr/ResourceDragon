#include "Render.h"
#include <Clipboard.h>
#include <DirectoryNode.h>
#include <Markdown.h>
#include <PreviewWindow.h>
#include <Themes.h>
#include <UIError.h>
#include "SDL3/SDL_video.h"
#include "imgui.h"
#include "imgui/misc/freetype/imgui_freetype.h"
#include "imgui_internal.h"
#include "state.h"
#include <fmt/core.h>
#include <SDK/util/Logger.hpp>

usize last_preview_index = -1;
std::string pending_focus_tab_name;

#include <filesystem>
namespace fs = std::filesystem;

#define DIRECTORY_LIST_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
#define FILE_PREVIEW_FLAGS   DIRECTORY_LIST_FLAGS | ImGuiWindowFlags_HorizontalScrollbar

void RenderFBContextMenu(ImGuiIO *io) {
    if (ImGui::BeginPopupContextWindow("FBContextMenu")) {
        if (ImGui::BeginMenu("Open As...")) {
            // Open fb__selectedItem as whatever is selected.
            if (ImGui::MenuItem("Text")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::TEXT, preview_index);
            }
            if (ImGui::MenuItem("Hex")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::HEX, preview_index);
            }
            if (ImGui::MenuItem("Image")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::IMAGE, preview_index);
            }
            if (ImGui::MenuItem("Audio")) {
                DirectoryNode::HandleFileClick(fb__selectedItem, ContentType::AUDIO, preview_index);
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
                if (fb__selectedItem->FullPath == GetPreviewState(preview_index).contents.path) {
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
    PreviewWinState &state = GetPreviewState(preview_index);
    if (ImGui::BeginPopupContextItem("PreviewItemContextMenu")) {
        if (ImGui::MenuItem("Copy to Clipboard")) {
            if (state.contents.size > 0) {
                if (state.contents.type == ContentType::IMAGE) {
                    Clipboard::CopyBufferToClipboard(state.contents.data, state.contents.size, state.contents.fileName);
                } else {
                    Clipboard::CopyFilePathToClipboard(state.contents.path);
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

ImFont* LoadFont(ImGuiIO& io, fs::path font_path, const char *font_name, const ImVector<ImWchar>& ranges, ImFontConfig *cfg = nullptr) {
    if (!cfg) {
        cfg = new ImFontConfig();
        cfg->OversampleH = 2;
        cfg->OversampleV = 2;
        cfg->SizePixels = 26;
    }
    if (fs::exists(font_path)) {
        auto font = io.Fonts->AddFontFromFileTTF(font_path.string().c_str(), cfg->SizePixels, cfg, ranges.Data);
        if (!font) {
            Logger::warn("Failed to load main font!");
        }
        font_registry[font_name] = font;
        return font;
    }
    return nullptr;
}

ImFont *LoadFont(ImGuiIO& io, fs::path font_path, const char *font_name, const ImVector<ImWchar>& ranges, float font_size) {
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.SizePixels = font_size;
    return LoadFont(io, font_path, font_name, ranges, &cfg);
}

static int tabCount = 0;

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
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 2));
    ImGui::DockSpace(root_id, ImVec2(0,0), dockspace_flags);
    ImGuiDockNode* node = ImGui::DockBuilderGetNode(root_id);
    if (node) {
        // Traverse dock nodes to find visible tab bars
        ImGuiDockContext* ctx = &GImGui->DockContext;
        for (int i = 0; i < ctx->Nodes.Data.Size; i++) {
            if (ImGuiDockNode* child = (ImGuiDockNode*)ctx->Nodes.Data[i].val_p) {
                if (child && child->TabBar && child->IsVisible) {
                    ImGuiTabBar* tab_bar = child->TabBar;
                    ImRect tab_rect = tab_bar->BarRect;

                    float height = tab_rect.GetHeight();
                    ImVec2 button_size(height * 0.8f, height * 0.8f);
                    ImGuiTabItem* last_tab = &tab_bar->Tabs.back();
                    float last_tab_end_x = last_tab->Offset + last_tab->Width;

                    ImVec2 button_pos(
                        tab_rect.Min.x + last_tab_end_x + ImGui::GetStyle().ItemSpacing.x,
                        tab_rect.Min.y + (height - button_size.y) * 0.5f
                    );

                    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
                    ImGui::SetCursorScreenPos(button_pos);

                    ImGui::SetNextWindowPos(button_pos);
                    ImGui::SetNextWindowBgAlpha(0.0f);
                    ImGui::Begin("##plus_overlay", nullptr,
                        ImGuiWindowFlags_NoDecoration |
                        ImGuiWindowFlags_NoNav |
                        ImGuiWindowFlags_NoFocusOnAppearing |
                        ImGuiWindowFlags_AlwaysAutoResize);

                    ImGui::SetCursorPos(ImVec2(0, 0));
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,0,0,0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0,0,0,0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0,0,0,0));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));

                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,180));
                    if (ImGui::Button("+", button_size)) {
                        std::string name = fmt::format("Preview###Preview_{}", tabCount);
                        ImGui::DockBuilderDockWindow(name.c_str(), child->ID);
                        preview_tabs.push_back(name);
                        preview_windows.push_back(PreviewWinState{});

                        preview_index = preview_tabs.size() - 1;
                        last_preview_index = preview_index;
                        ImGui::DockBuilderDockWindow(name.c_str(), child->ID);
                        pending_focus_tab_name = name;
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        ImGui::PopStyleColor();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                    }

                    ImGui::PopStyleColor(4);
                    ImGui::PopStyleVar();

                    ImGui::End();
                }
            }
        }
    }
    ImGui::PopStyleVar(1);

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

    // Logger::LogTest();

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
    const char *noto_path_bold = "/usr/share/fonts/noto-cjk/NotoSansCJK-Bold.ttc";
    if (fs::exists(noto_path_bold)) {
        md_font_bold_large = LoadFont(io, noto_path_bold, "UIFontBoldLarge", gr, 32 * 1.5);
        md_font_bold_medium = LoadFont(io, noto_path_bold, "UIFontBoldMedium", gr, 24 * 1.5);
        md_font_bold = LoadFont(io, noto_path_bold, "UIFontBold", gr, 20 * 1.5);
    };
    if (fs::exists(linux_font_path)) {
        noto_font_path = fs::path(linux_font_path);
    };

#endif
    auto ui_font = LoadFont(io, noto_font_path, "UIFont", gr);
    io.FontDefault = ui_font;
    md_font_regular = ui_font;

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

    ImGui::PushFont(font_registry.find("UIFont")->second);

    // Auto-open file if passed as command-line argument
    static bool initial_file_opened = false;
    if (!initial_file_opened && !initial_file_to_open.empty() && rootNode) {
        // Find the file node in the rootNode's children
        for (auto* child : rootNode->Children) {
            if (child->FullPath == initial_file_to_open) {
                HandleFileClick(child, ContentType::UNKNOWN, preview_index);
                initial_file_opened = true;
                break;
            }
        }
    }

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

        for (size_t i = 0; i < preview_tabs.size(); ) {
            auto &tab = preview_tabs[i];
            bool open = true;

            bool is_first_tab = (i == 0);
            if (is_first_tab)
                ImGui::Begin(tab.c_str(), nullptr, FILE_PREVIEW_FLAGS);
            else
                ImGui::Begin(tab.c_str(), &open, FILE_PREVIEW_FLAGS);

            if (!pending_focus_tab_name.empty() && pending_focus_tab_name == tab) {
                ImGui::SetWindowFocus();
                preview_index = i;
                last_preview_index = i;
                pending_focus_tab_name.clear();
            }

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                preview_index = i;

            PreviewWinState &state = GetPreviewState(i);
            PreviewContextMenu();

            if (preview_index == i && last_preview_index != preview_index) {
                if ((state.contents.type == TEXT || state.contents.type == MARKDOWN) &&
                    state.contents.data && state.contents.size > 0) {
                    editor.SetText(std::string((char*)state.contents.data, state.contents.size));
                    editor.SetTextChanged(false);
                }
                last_preview_index = preview_index;
            }

            if (state.contents.size > 0) {
                PreviewWindow::RenderPreviewFor(state.contents.type);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                    ImGui::OpenPopup("PreviewItemContextMenu");
            } else {
                ImGui::Text("No file selected.");
            }

            ImGui::End();

            if (!is_first_tab && !open) {
                preview_tabs.erase(preview_tabs.begin() + i);
                preview_windows.erase(preview_windows.begin() + i);

                if (preview_index > 0 && preview_index >= i)
                    preview_index--;
                if (last_preview_index > 0 && last_preview_index >= i)
                    last_preview_index--;
            } else {
                ++i;
            }
        }


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

    PreviewWinState &state = GetPreviewState(preview_index);

    MIX_DestroyAudio(state.audio.music);

    SDL_DestroyWindow(window);
    SDL_RemoveTimer(state.audio.update_timer);
    SDL_Quit();
}
