namespace Plugins {
    typedef void (*RD_PluginInit)();
    typedef void (*RD_PluginShutdown)();

    void LoadPlugins(const char *path);
}
