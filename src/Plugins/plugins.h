#include <string>
#include <vector>

#include "../SDK/ArchiveFormatWrapper.h"
#include "../SDK/sdk.h"

namespace Plugins {

    typedef const ArchiveFormatVTable* (*RD_GetArchiveFormat)(struct sdk_ctx* ctx);
    typedef sdk_ctx* (*RD_PluginInit)();
    typedef void (*RD_PluginShutdown)();

    struct Plugin {
        std::string name;
        std::string path;
        RD_PluginInit init;
        RD_PluginShutdown shutdown;
        RD_GetArchiveFormat getArchiveFormat;
    };

    static std::vector<Plugin> plugins;

    void LoadPlugins(const char *path);
    void Shutdown();
}
