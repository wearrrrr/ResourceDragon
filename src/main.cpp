#include <filesystem>
#include <fstream>
#include "ArchiveFormats/HSP/hsp.h"
#include "ExtractorManager.h"

#include <SDL3/SDL.h>

#include "imgui.h"
#include "../vendored/imgui/imgui_impl_sdl3.h"
#include "../vendored/imgui/imgui_impl_opengl3.h"

#include "GUI/Theme/Themes.h"
#include "GUI/DirectoryNode.h"
#include "GUI/Image.h"
#include "GUI/Utils.h"

#define DEBUG

namespace fs = std::filesystem;

static ExtractorManager extractor_manager;

static DirectoryNode rootNode;

void ReloadRootNode(DirectoryNode& node)
{
    rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(node.FullPath));
}

void ChangeDirectory(DirectoryNode& node, DirectoryNode& rootNode)
{
    std::filesystem::path newRootPath(node.FullPath);

    if (node.FileName == "..") {
        std::filesystem::path parentPath = std::filesystem::path(rootNode.FullPath).parent_path();
        if (parentPath != std::filesystem::path(rootNode.FullPath)) {
            newRootPath = parentPath;
        } else {
            return;
        }
    }
    rootNode = CreateDirectoryNodeTreeFromPath(newRootPath);
}


#ifdef DEBUG
#define FPS_OVERLAY_FLAGS ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs
#endif

#define DIRECTORY_TREE_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
#define FILE_PREVIEW_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_HorizontalScrollbar

struct PreviewWindowState {
    char *rawContents = nullptr;
    long rawContentsSize = 0;
    std::string rawContentsExt;
    struct Texture {
        GLuint id;
        struct {
            int x;
            int y;
        } size;
    } texture;
};

static PreviewWindowState preview_state = {
    .rawContents = nullptr,
    .rawContentsSize = 0,
    .rawContentsExt = "",
    .texture = {
        .id = 0,
        .size = {0, 0}
    }
};

void HandleFileClick(DirectoryNode& node)
{
    std::string filename = node.FileName;
    std::string ext = filename.substr(filename.find_last_of(".") + 1);

    auto [buffer, size] = read_file_to_buffer<unsigned char>(node.FullPath.c_str());

    ArchiveFormat *format = extractor_manager.getExtractorFor(buffer, size, ext);

    if (format != nullptr) {
        printf("Format: %s\n", format->getTag().c_str());
        ArchiveBase *arc = (ArchiveBase*)format->TryOpen(buffer, size);
        fs::remove_all("decrypt/");
        fs::create_directory("decrypt");

        for (int i = 0; i < arc->entries.size(); i++) {
            Entry entry = arc->entries.at(i);
            const char *data = arc->OpenStream(entry, buffer);
            std::ofstream outFile("decrypt/" + entry.name, std::ios::binary);
            outFile.write((const char*)data, entry.size);
            outFile.close();
        }
        printf("Decrypted successfully!\n");

        ReloadRootNode(rootNode);

        delete arc;

        free(buffer);
    } else {
        if (preview_state.rawContents) {
            free(preview_state.rawContents);
            preview_state.rawContents = nullptr;
        }
        Image::UnloadTexture(preview_state.texture.id);
        preview_state.texture.size.x = 0;
        preview_state.texture.size.y = 0;

        preview_state.rawContents = (char*)buffer;
        preview_state.rawContentsSize = size;
        preview_state.rawContentsExt = ext;

        if (Image::IsImageExtension(ext)) {
            Image::LoadTextureFromMemory(preview_state.rawContents, preview_state.rawContentsSize, &preview_state.texture.id, &preview_state.texture.size.x, &preview_state.texture.size.y);
        }
    }
}


void DisplayDirectoryNodeRecursive(DirectoryNode& node, DirectoryNode& rootNode)
{
    ImGui::TableNextRow();
    ImGui::PushID(&node);

    bool directoryClicked = false;
    bool fileClicked = false;

    ImGui::TableNextColumn();

    bool isRoot = (&node == &rootNode);
    
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (node.IsDirectory) {
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;
    } else {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (isRoot) { 
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing()); 
    };

    bool isOpen = ImGui::TreeNodeEx(node.FileName.c_str(), nodeFlags);
    
    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node.IsDirectory) directoryClicked = true;
        else fileClicked = true;
    }

    ImGui::TableNextColumn();
    if (!node.IsDirectory) {
        ImGui::Text("%s", Utils::GetFileSize(node.FullPath).c_str());
    } else {
        ImGui::Text("--");
    }


    ImGui::TableNextColumn();
    auto ftime = Utils::GetLastModifiedTime(node.FullPath);
    ImGui::Text("%s", ftime.c_str());

    if (node.IsDirectory && isOpen) {
        for (auto& childNode : node.Children) {
            DisplayDirectoryNodeRecursive(childNode, rootNode);
        }
        ImGui::TreePop();
    }

    if (fileClicked) {
        HandleFileClick(node);
    } else if (directoryClicked) {
        ChangeDirectory(node, rootNode);
    }

    ImGui::PopID();
}


void DisplayDirectoryNode(DirectoryNode& node, DirectoryNode& rootNode, bool isRoot = false)
{
    ImGui::PushID(&node);

    ImGui::BeginTable("DirectoryTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentDisable);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 215.0f);
    ImGui::TableHeadersRow();

    DisplayDirectoryNodeRecursive(node, rootNode);

    ImGui::EndTable();

    ImGui::PopID();
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
    ImGuiIO& io = ImGui::GetIO();
    ImFontGlyphRangesBuilder range;

    ImVector<ImWchar> gr;
    range.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    // range.AddRanges(io.Fonts->GetGlyphRangesKorean());
    range.BuildRanges(&gr);

    io.Fonts->AddFontFromFileTTF("fonts/NotoSansCJK-Medium.ttc", 28, nullptr, gr.Data);

    io.DeltaTime = 0.01667;
    Theme::SetTheme("BessDark");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetSwapInterval(1);

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_ShowWindow(window);

    bool running = true;
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    while (running) {
        ImVec2 window_size = ImVec2(io.DisplaySize.x, io.DisplaySize.y);
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
        
        ImGui::NewFrame();
        ImGui::SetNextWindowSize({window_size.x / 2 + 150, window_size.y});
        ImGui::SetNextWindowPos({0, 0});
        if (ImGui::Begin("Directory Tree", NULL, DIRECTORY_TREE_FLAGS)) {
            DisplayDirectoryNode(rootNode, rootNode, true);
        }
        ImGui::End();

        ImGui::SetNextWindowSize({window_size.x / 2 - 150, window_size.y});
        ImGui::SetNextWindowPos({window_size.x / 2 + 150, 0});
        if(ImGui::Begin("Preview", NULL, FILE_PREVIEW_FLAGS)) {
            if (preview_state.rawContentsSize > 0) {
                // TODO: Only the first frame is rendered when loading a gif.
                if (Image::IsImageExtension(preview_state.rawContentsExt)) {
                    ImVec2 image_size = ImVec2(preview_state.texture.size.x, preview_state.texture.size.y);
                    if (preview_state.texture.id) {
                        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - image_size.x) * 0.5f, 75));
                        ImGui::Image(preview_state.texture.id, image_size);
                    } else {
                        ImGui::Text("Failed to load image!");
                    }
                } else {
                    // TODO: handle different potential encodings using a dropdown for the user to select the encoding.
                    char* text_end = preview_state.rawContents + preview_state.rawContentsSize;
                    ImGui::TextUnformatted(preview_state.rawContents, text_end);
                }
            } else {
                ImGui::Text("No file selected.");
            }
        }
        ImGui::End();

        const float DISTANCE = 8.0f;
        ImVec2 window_pos = ImVec2(DISTANCE, window_size.y - DISTANCE);
        ImVec2 window_pos_pivot = ImVec2(0.0f, 1.0f);
    
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.55f);

        #ifdef DEBUG
        if (ImGui::Begin("FPS Overlay", nullptr, FPS_OVERLAY_FLAGS)) 
        {
            ImGui::Text("FPS: %.1f", io.Framerate);
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