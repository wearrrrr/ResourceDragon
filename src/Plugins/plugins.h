#include <string>
#include <vector>

namespace Plugins {

    typedef void (*RD_PluginInit)();
    typedef void (*RD_PluginShutdown)();

    struct Plugin {
        std::string name;
        std::string path;
        RD_PluginInit init;
        RD_PluginShutdown shutdown;
    };

    static std::vector<Plugin> plugins;

    void LoadPlugins(const char *path);
    void Shutdown();
}
