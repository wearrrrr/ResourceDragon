#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <iostream>
#include "ArchiveFormats/HSP/hsp.h"
#include "ExtractorManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include "imgui.h"
#include "GUI/DirectoryNode.h"
#include "../vendored/imgui/imgui_impl_sdl3.h"
#include "../vendored/imgui/imgui_impl_opengl3.h"

#include "GUI/Theme/Themes.h"

namespace fs = std::filesystem;

static ExtractorManager extractor_manager;

void DisplayDirectoryNode(DirectoryNode& node, DirectoryNode& rootNode, bool isRoot = false)
{
    ImGui::PushID(&node);

    bool directoryClicked = false;
    bool fileClicked = false;

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (node.IsDirectory) {
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;
    } else {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (isRoot) nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool isOpen = ImGui::TreeNodeEx(node.FileName.c_str(), nodeFlags);

    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node.IsDirectory) directoryClicked = true;
        else fileClicked = true;
    }

    if (node.IsDirectory && isOpen) {
        for (auto &childNode : node.Children) {
            DisplayDirectoryNode(childNode, rootNode, false);
        }
        ImGui::TreePop();
    }

    if (fileClicked) {
        std::string filename = node.FileName;
        std::string ext = filename.substr(filename.find_last_of(".") + 1);
        
        printf("File name: %s\n", filename.c_str());
        printf("File extension: %s\n", ext.c_str());

        auto [buffer, size] = read_file_to_buffer<unsigned char>(node.FullPath.c_str());
        printf("Size: %ld\n", size);

        ArchiveFormat *format = extractor_manager.getExtractorFor(buffer, size);

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
        } else {
            printf("No compatible format found!\n");
        }

        
    }

    if (directoryClicked) {
        std::filesystem::path newRootPath = node.FullPath;

        if (node.FileName == "..") {
            newRootPath = std::filesystem::path(rootNode.FullPath).parent_path();
            if (node.FileName == "..") {
                std::filesystem::path parentPath = std::filesystem::path(rootNode.FullPath).parent_path();
                if (parentPath != rootNode.FullPath) {
                    newRootPath = parentPath;
                } else {
                    ImGui::PopID();
                    return;
                }
            }
            if (newRootPath.empty() || newRootPath == rootNode.FullPath) {
                ImGui::PopID();
                return;
            }
        }

        if (!newRootPath.empty()) {
            rootNode = CreateDirectoryNodeTreeFromPath(newRootPath);
        }
    }

    ImGui::PopID();
}




int main(int argc, char* argv[]) {

    extractor_manager.registerFormat(std::make_unique<HSPArchive>());

    static DirectoryNode rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical("./"));

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    SDL_Window* window = SDL_CreateWindow("ResourceDragon", 1280, 720, window_flags);
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

    const char* glsl_version = "#version 130";

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("fonts/NotoSansCJK-Medium.ttc", 30);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DeltaTime = 0.01667;
    Theme::BessDark();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_ShowWindow(window);

    bool running = true;
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                running = false;
        }
        
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowSize({io.DisplaySize.x / 2, io.DisplaySize.y});
        ImGui::SetNextWindowPos({0, 0});
        if (ImGui::Begin("Directory Tree", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
            DisplayDirectoryNode(rootNode, rootNode, true);
        }
        ImGui::End();

        ImGui::SetNextWindowSize({io.DisplaySize.x / 2, io.DisplaySize.y});
        ImGui::SetNextWindowPos({io.DisplaySize.x / 2, 0});
        ImGui::Begin("Preview", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing);
        
        ImGui::End();
        


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