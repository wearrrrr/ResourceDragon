#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

namespace GUI {
    bool InitRendering();
    void RenderLoop();
}
