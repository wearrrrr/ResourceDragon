#include <stdio.h>

extern "C" {
    const char *RD_PluginName = "TestPlugin";
    const char *RD_PluginVersion = "0.1";
    void RD_PluginInit() {
        printf("Plugin initialized! (From the plugin)\n");
    }
    void RD_PluginShutdown() {
        printf("Plugin shutdown\n");
    }
}
