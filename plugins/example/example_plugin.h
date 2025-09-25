#include "../../SDK/sdk.h"
#include <vector>
#include <string>

extern "C" {
    RD_EXPORT const char* RD_PluginName;
    RD_EXPORT const char* RD_PluginVersion;
}

struct Entry {
    std::string name;
    std::vector<u8> content;
};

struct DemoArchive {
    std::vector<Entry> entries;
};

RD_EXPORT bool RD_PluginInit(HostAPI* api);
RD_EXPORT void RD_PluginShutdown();
RD_EXPORT const ArchiveFormatVTable* RD_GetArchiveFormat(sdk_ctx* ctx);
