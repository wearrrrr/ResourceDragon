#include "SDL3/SDL_hints.h"
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <ArchiveFormats/HSP/hsp.h>
#include <ArchiveFormats/PFS/pfs.h>
#include <ArchiveFormats/NitroPlus/nitroplus.h>
#include <ArchiveFormats/SonicAdv/sonicadv.h>
#include <ArchiveFormats/Touhou/pbg.h>
#include <ArchiveFormats/XP3/xp3.h>

#include <GUI/Audio.h>
#include <GUI/Render.h>
#include <GUI/Themes.h>
#include <GUI/DirectoryNode.h>
#include <GUI/Clipboard.h>
#include <GUI/PreviewWindow.h>
#include <GUI/UIError.h>
#include <Scripting/ScriptManager.h>
#include <thread>
#include <filesystem>

#include "state.h"

template <class T>
inline void RegisterFormat() {
    extractor_manager->RegisterFormat(std::make_unique<T>());
}

template <class T>
inline void RegisterFormat(T *arc_fmt) {
    extractor_manager->RegisterFormat(std::unique_ptr<T>(arc_fmt));
}

int main(int argc, char *argv[]) {
    RegisterFormat<HSPArchive>();
    RegisterFormat<PFSFormat>();
    RegisterFormat<NitroPlus::NPK>();
    RegisterFormat<NitroPlus::MPK>();
    RegisterFormat<SonicAdv::PAK>();
    RegisterFormat<THDAT>();
    RegisterFormat<XP3Format>();

    ScriptManager *scriptManager = new ScriptManager();

    try {
        for (const auto &entry : fs::directory_iterator("scripts/")) {
            const fs::path entry_path = entry.path();
            if (entry_path.extension() == ".nut") {
                if (scriptManager->LoadFile(entry_path.string())) {
                    auto fmt = scriptManager->Register();
                    if (fmt) RegisterFormat<SquirrelArchiveFormat>(fmt);
                }
            }
        }
    } catch (const fs::filesystem_error &err) {
        Logger::error("Failed to start scripting! Error: %s", err.what());
    }

#ifdef __linux__
    // Clear temp dir on startup, this invalidates a file copied to the clipboard from a previous run, but that's fine i guess.
    fs::remove_all("/tmp/rd/");
#endif

    const char *path;
    if (argc < 2) {
        path = ".";
    } else {
        path = argv[1];
    }

#ifdef __linux__
    #define INOTIFY_FLAGS IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE
    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        Logger::error("inotify_init() failed!");
        return -1;
    }
    inotify_wd = inotify_add_watch(inotify_fd, path, INOTIFY_FLAGS);
    if (inotify_wd < 0) {
        Logger::error("inotify_add_watch() failed!");
        close(inotify_fd);
        return -1;
    }
    bool inotify_running = true;
    std::thread inotify_thread([=]() {
        char buffer[1024];
        while (inotify_running) {
            int length = read(inotify_fd, buffer, sizeof(buffer));
            if (length < 0) {
                Logger::error("read() failed from inotify_fd!");
                break;
            }
            int i = 0;
            while (i < length) {
                inotify_event *event = (inotify_event*)&buffer[i];
                if (event->mask & INOTIFY_FLAGS) {
                    ReloadRootNode(rootNode);
                }
                i += EVENT_SIZE + event->len;
            }
        }
    });
    inotify_thread.detach();
#endif

    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY, "1");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        Logger::error("Error: SDL_Init(): %s\n", SDL_GetError());
    }

    Audio::InitAudioSystem();

#ifdef DEBUG
    Logger::print_stacktrace("Test error with stacktrace..");
#endif

    if (GUI::InitRendering()) {
        GUI::StartRenderLoop(path);
    }

    DirectoryNode::Unload(rootNode);

#ifdef linux
    if (inotify_thread.joinable()) {
        inotify_thread.join();
    }
    inotify_running = false;
    close(inotify_fd);
    #endif



    return 0;
}
