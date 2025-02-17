#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <iostream>
#include "ArchiveFormats/HSP/hsp.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include "imgui.h"
#include "GUI/DirectoryNode.h"
#include "../vendored/imgui/imgui_impl_sdl3.h"
#include "../vendored/imgui/imgui_impl_opengl3.h"


namespace fs = std::filesystem;

std::filesystem::path pendingRootPath;

void RecursivelyDisplayDirectoryNode(DirectoryNode& parentNode, DirectoryNode& rootNode, bool isRoot = false)
{
    ImGui::PushID(&parentNode);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (parentNode.IsDirectory) {
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
    } else {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (isRoot) {
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool isOpen = ImGui::TreeNodeEx(parentNode.FileName.c_str(), nodeFlags);

    if (parentNode.FullPath != rootNode.FullPath) {
        ImGui::SetNextItemOpen(false);
    }

    if (parentNode.IsDirectory && isOpen) {
        for (auto &childNode : parentNode.Children) {
            RecursivelyDisplayDirectoryNode(childNode, rootNode, false);
        }
        ImGui::TreePop();
    }

    if (parentNode.IsDirectory && ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        std::filesystem::path newRootPath = parentNode.FullPath;

        if (parentNode.FileName == "..") {
            newRootPath = std::filesystem::path(rootNode.FullPath).parent_path();
            if (parentNode.FileName == "..") {
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

        if (!newRootPath.empty() && newRootPath != pendingRootPath) {
            pendingRootPath = newRootPath;
        }
    }

    ImGui::PopID();
}




int main(int argc, char* argv[]) {

    // TODO: make this more graceful, likely default to home dir and just load things when the tree is expanded instead of trying to load everything at once.
    static DirectoryNode rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical("./"));

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    SDL_Window* window = SDL_CreateWindow("ResourceDragon", 800, 600, window_flags);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("fonts/NotoSansCJK-Medium.ttc", 30);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DeltaTime = 0.01667;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // vsync
    SDL_ShowWindow(window);

    bool running = true;

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);

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
        ImGui::SetNextWindowSize({io.DisplaySize.x, 500});
        ImGui::SetNextWindowPos({0, 100});
        if (ImGui::Begin("Directory Tree", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            RecursivelyDisplayDirectoryNode(rootNode, rootNode, true);
        }
        ImGui::End();
        
        ImGui::Render();

        if (!pendingRootPath.empty())
        {
            rootNode = CreateDirectoryNodeTreeFromPath(pendingRootPath);
            pendingRootPath.clear();
        }

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);

        SDL_Delay(12);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    HSPArchive *arc = new HSPArchive();
    auto [buffer, size] = arc->open("./eXceed3.exe");
    DPMArchive *opened_arc = arc->TryOpen(buffer, size);
    DPMEntry entry = opened_arc->entries.at(0);

    if (entry.offset + entry.size > size) {
        printf("Entry is out of bounds! This is very bad.\n");
        return 1;
    }

    fs::remove_all("decrypt/");
    fs::create_directory("decrypt");

    for (int i = 0; i < opened_arc->entries.size(); i++) {
        DPMEntry entry = opened_arc->entries.at(i);
        const char *data = opened_arc->OpenStream(entry, buffer);
        std::ofstream outFile("decrypt/" + entry.name, std::ios::binary);
        outFile.write((const char*)data, entry.size);
        outFile.close();
    }
    printf("Decrypted successfully!\n");
    
    return 0;
}