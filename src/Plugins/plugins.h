#include <string>
#include <vector>

#include "../../SDK/sdk.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Plugins {

    typedef const ArchiveFormatVTable* (*RD_GetArchiveFormat_t)(struct sdk_ctx* ctx);
    typedef bool (*RD_PluginInit_t)(HostAPI* api);
    typedef void (*RD_PluginShutdown_t)();

    struct Plugin {
        std::string name;
        std::string path;
#ifdef _WIN32
        HMODULE handle;
#else
        void *handle;
#endif
        RD_PluginInit_t init;
        RD_PluginShutdown_t shutdown;
        RD_GetArchiveFormat_t getArchiveFormat;
        sdk_ctx *ctx;
    };

    static std::vector<Plugin> plugins;

    void LoadPlugins(const char *path);
    void Shutdown();
}
