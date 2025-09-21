#include <SDL3/SDL.h>

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
#include <Plugins/plugins.h>

#include <thread>
#include <filesystem>

#include "state.h"

#if defined(__linux__) && defined(DEBUG)
#include <unistd.h>
#include <signal.h>
#include <util/Stacktrace.h>

// TODO: log crash information to a file based on user settings
static void crash_handler(int sig, siginfo_t* si, void* ucontext) {
    pid_t pid = fork();
    if (pid < 0) {
        const char msg[] = "Crash: fork failed\n";
        write(STDERR_FILENO, msg, sizeof(msg)-1);
        _exit(128 + sig);
    }

    if (pid == 0) {
        signal(SIGSEGV, SIG_DFL);
        auto message = "Crash detected! Dumping trace:\n";
        write(STDERR_FILENO, message, strlen(message));
        auto trace = Stacktrace::generate_stacktrace().to_string();
        trace += "\n";
        write(STDERR_FILENO, trace.c_str(), trace.size());
        _exit(128 + sig);
    }
}

static void install_crash_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
}
#endif

template <class T>
inline void RegisterFormat() {
    extractor_manager->RegisterFormat(std::make_unique<T>());
}

int main(int argc, char** argv) {
#if defined(__linux__) && defined(DEBUG)
    install_crash_handler();
#endif

    const char *path;
    if (argc < 2) {
        path = ".";
    } else {
        if (strcmp(argv[1], "--version") == 0) {
            printf("ResourceDragon v%s\n", APP_VERSION);
            printf("Copyright (c) 2025 wearr (https://github.com/wearrrrr)\n");
            printf("Source Code: https://github.com/wearrrrr/ResourceDragon\n\n");
            printf("License: MIT\n");
            printf("https://github.com/wearrrrr/ResourceDragon/blob/master/LICENSE\n");

            return 0;
        }
        path = argv[1];
    }

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
                    if (fmt) {
                        extractor_manager->RegisterFormat(std::unique_ptr<SquirrelArchiveFormat>(fmt));
                    }
                }
            }
        }
    } catch (const fs::filesystem_error &err) {
        Logger::error("Failed to start scripting! Error: {}", err.what());
    }

    Plugins::LoadPlugins("plugins/");

#ifdef __linux__
    // Clear temp dir on startup, this invalidates a file copied to the clipboard from a previous run, but that's fine i guess.
    fs::remove_all("/tmp/rd/");
#endif

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

    // I know this hint only does anything on linux anyways, but i'm not going to have a repeat of the QT experience.
#ifdef __linux__
    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY, "1");
#endif

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        Logger::error("Error: SDL_Init(): {}", SDL_GetError());
    }

    Audio::InitAudioSystem();

    auto canonical_path = fs::canonical(path).string();

    SetFilePath(canonical_path);
    rootNode = DirectoryNode::CreateTreeFromPath(canonical_path);

    if (GUI::InitRendering()) {
        GUI::StartRenderLoop();
    }

    Plugins::Shutdown();

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
