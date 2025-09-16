#include "plugins.h"
#include <util/Logger.h>


#include <string>
#include <filesystem>

namespace fs = std::filesystem;

using namespace Plugins;

#ifdef __linux__

#include <dlfcn.h>

// bad solution but i'll fix it later, ideally we will read the elf header to check.
bool is_shared_library(const fs::path &p) {
    if (!fs::is_regular_file(p)) return false;
    std::string name = p.filename().string();
    if (name == ".so") return true;

    auto pos = name.find(".so");
    if (pos == std::string::npos) return false;
    return pos + 3 == name.size() || name[pos + 3] == '.';
}

void Plugins::LoadPlugins(const char *path) {
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file() && is_shared_library(entry)) {
            void* handle = dlopen(entry.path().string().c_str(), RTLD_LAZY);
            if (!handle) {
                Logger::error("Failed to load plugin");
                continue;
            }
            RD_PluginInit init = (RD_PluginInit)dlsym(handle, "RD_PluginInit");
            if (!init) {
                Logger::error("Failed to find RD_PluginInit symbol in plugin");
                dlclose(handle);
                continue;
            }
            const char **name = (const char **)dlsym(handle, "RD_PluginName");
            if (!name) {
                Logger::error("Failed to find RD_PluginName symbol in plugin");
                dlclose(handle);
                continue;
            }
            const char **version = (const char **)dlsym(handle, "RD_PluginVersion");
            if (!version) {
                Logger::error("Failed to find RD_PluginVersion symbol in plugin");
                dlclose(handle);
                continue;
            }
            RD_PluginShutdown shutdown = (RD_PluginShutdown)dlsym(handle, "RD_PluginShutdown");
            if (!shutdown) {
                Logger::error("Failed to find RD_PluginShutdown symbol in plugin");
                dlclose(handle);
                continue;
            }
            printf("Plugin %s (v%s) loaded\n", *name, *version);
            init();
        }
    }
}

#else

void Plugins::LoadPlugins(const char *path) {
    Logger::error("Plugin loading currently not supported on this platform!");
}

#endif
