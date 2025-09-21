#include "plugins.h"
#include "state.h"
#include <util/Logger.h>
#include "../SDK/sdk.h"
#include "../SDK/ArchiveFormatWrapper.h"

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

using namespace Plugins;

#ifdef __linux__

#include <dlfcn.h>

struct sdk_ctx {
    int version;
    Logger* logger;
    ArchiveFormatWrapper *archiveFormat;
};

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
    try {
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

                RD_GetArchiveFormat getArchiveFormat = (RD_GetArchiveFormat)dlsym(handle, "RD_GetArchiveFormat");
                if (!getArchiveFormat) {
                    Logger::error("Failed to find RD_GetArchiveFormat symbol in plugin");
                    dlclose(handle);
                    continue;
                }

                Plugin plugin = {
                    *name,
                    entry.path().string(),
                    init,
                    shutdown,
                    getArchiveFormat
                };

                sdk_ctx *sdk_ctx = init();
                plugin.ctx = sdk_ctx;
                plugins.push_back(plugin);
                printf("Plugin %s (v%s) loaded\n", *name, *version);

                const ArchiveFormatVTable* vtable = getArchiveFormat(sdk_ctx);
                if (vtable) {
                    ArchiveFormatWrapper* wrapper = AddArchiveFormat(sdk_ctx, vtable);
                    if (wrapper) {
                        extractor_manager->RegisterFormat(std::unique_ptr<ArchiveFormatWrapper>(wrapper));
                    } else {
                        Logger::error("Failed to create archive format wrapper!");
                    }
                } else {
                    Logger::error("Failed to get archive format vtable from plugin!");
                }

            }
        }
    } catch (fs::filesystem_error &e) {
        Logger::error("Failed to load plugins: {}", e.what());
    }
}

void Plugins::Shutdown() {
    for (auto &plugin : plugins) {
        plugin.shutdown();
        sdk_deinit(plugin.ctx);
    }
}

#else

void Plugins::LoadPlugins(const char *path) {
    Logger::error("Plugin loading currently not supported on this platform!");
}

#endif
