#include "plugins.h"
#include "../state.h"
#include <util/Logger.h>
#include "../SDK/sdk.h"
#include "../SDK/ArchiveFormatWrapper.h"

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

using namespace Plugins;

// Global SDK context for plugins
static sdk_ctx* global_ctx = nullptr;

// bad solution but i'll fix it later, ideally we will read the elf header to check.
bool is_shared_library(const fs::path &p) {
    if (!fs::is_regular_file(p)) return false;
    std::string name = p.filename().string();
    if (name == ".so") return true;

    auto pos = name.find(".so");
    if (pos == std::string::npos) return false;
    return pos + 3 == name.size() || name[pos + 3] == '.';
}

#ifdef _WIN32
using LibHandle = HMODULE;
inline LibHandle LoadLib(const char* path) {
    return LoadLibraryA(path);
}
inline void* GetSym(LibHandle h, const char* sym) {
    return reinterpret_cast<void*>(GetProcAddress(h, sym));
}
inline void CloseLib(LibHandle h) {
    FreeLibrary(h);
}
#else
using LibHandle = void*;
inline LibHandle LoadLib(const char* path) {
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}
inline void* GetSym(LibHandle h, const char* sym) {
    return dlsym(h, sym);
}
inline void CloseLib(LibHandle h) {
    dlclose(h);
}
#endif

void Plugins::LoadPlugins(const char* path) {
    // Initialize global SDK context if not already done
    if (!global_ctx) {
        global_ctx = new sdk_ctx();
        sdk_init(global_ctx);
    }

    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_regular_file())
                continue;

#ifdef _WIN32
            if (entry.path().extension() != ".dll") continue;
#else
            if (!is_shared_library(entry)) continue;
#endif

            LibHandle handle = LoadLib(entry.path().string().c_str());
            if (!handle) {
                Logger::error("Failed to load plugin: {}", entry.path().string());
                continue;
            }

            auto init = reinterpret_cast<RD_PluginInit_t>(GetSym(handle, "RD_PluginInit"));
            auto shutdown = reinterpret_cast<RD_PluginShutdown_t>(GetSym(handle, "RD_PluginShutdown"));
            auto getArchiveFormat = reinterpret_cast<RD_GetArchiveFormat_t>(GetSym(handle, "RD_GetArchiveFormat"));
            auto name = reinterpret_cast<const char**>(GetSym(handle, "RD_PluginName"));
            auto version = reinterpret_cast<const char**>(GetSym(handle, "RD_PluginVersion"));

            if (!init || !shutdown || !getArchiveFormat || !name || !version) {
                Logger::error("Invalid plugin: {}", entry.path().string());
                Logger::error("Has Init? {}", init != nullptr);
                Logger::error("Has Shutdown? {}", shutdown != nullptr);
                Logger::error("Has GetArchiveFormat? {}", getArchiveFormat != nullptr);
                Logger::error("Has Name? {}", name != nullptr);
                Logger::error("Has Version? {}", version != nullptr);
                CloseLib(handle);
                continue;
            }

            Plugin plugin = {};
            plugin.name = *name;
            plugin.path = entry.path().string();
            plugin.handle = handle;
            plugin.init = init;
            plugin.shutdown = shutdown;
            plugin.getArchiveFormat = getArchiveFormat;
            plugin.ctx = global_ctx;

            HostAPI host_api = {};
            host_api.get_sdk_context = []() -> sdk_ctx* { return global_ctx; };
            host_api.log = [](sdk_ctx* ctx, const char* msg){
                if (ctx && ctx->logger) {
                    ctx->logger->log(msg);
                }
            };
            host_api.warn = [](sdk_ctx* ctx, const char* msg){
                if (ctx && ctx->logger) {
                    ctx->logger->warn(msg);
                }
            };
            host_api.error = [](sdk_ctx* ctx, const char* msg){
                if (ctx && ctx->logger) {
                    ctx->logger->error(msg);
                }
            };

            if (!init(&host_api)) {
                Logger::error("Plugin {} failed to init", entry.path().string());
                CloseLib(handle);
                continue;
            }

            plugins.push_back(plugin);

            printf("Plugin %s (v%s) loaded\n", *name, *version);

            const ArchiveFormatVTable* vtable = getArchiveFormat(global_ctx);
            if (vtable) {
                if (ArchiveFormatWrapper* wrapper = AddArchiveFormat(global_ctx, vtable)) {
                    extractor_manager->RegisterFormat(std::unique_ptr<ArchiveFormatWrapper>(wrapper));
                } else {
                    Logger::error("Failed to create archive format wrapper!");
                }
            } else {
                Logger::error("Failed to get archive format vtable from plugin!");
            }
        }
    } catch (fs::filesystem_error& e) {
        Logger::error("Failed to load plugins: {}", e.what());
    }
}

void Plugins::Shutdown() {
    for (auto &plugin : plugins) {
        if (plugin.shutdown) {
            plugin.shutdown();
        }
        CloseLib(plugin.handle);
    }
    plugins.clear();

    if (global_ctx) {
        sdk_deinit(global_ctx);
        delete global_ctx;
        global_ctx = nullptr;
    }
}
