#include "../../SDK/sdk.h"
#include <vector>
#include <string>
#include <cstring>

struct Entry {
    std::string name;
    std::vector<u8> content;
};

// Plugin metadata
const char* RD_PluginName = "Vector Demo Plugin";
const char* RD_PluginVersion = "1.0.0";

// -------------------- Demo Archive --------------------
struct DemoArchive {
    std::vector<Entry> entries;
};

// -------------------- Base vtable functions --------------------
static usize GetEntryCount(ArchiveInstance inst) {
    auto* arc = static_cast<DemoArchive*>(inst);
    return arc ? arc->entries.size() : 0;
}

static const char* GetEntryName(ArchiveInstance inst, usize idx) {
    auto* arc = static_cast<DemoArchive*>(inst);
    if (!arc || idx >= arc->entries.size()) return nullptr;
    return arc->entries[idx].name.c_str();
}

static usize GetEntrySize(ArchiveInstance inst, usize idx) {
    auto* arc = static_cast<DemoArchive*>(inst);
    if (!arc || idx >= arc->entries.size()) return 0;
    return arc->entries[idx].content.size();
}

static u8* OpenStream(ArchiveInstance inst, usize idx, usize* out_size) {
    auto* arc = static_cast<DemoArchive*>(inst);
    if (!arc || idx >= arc->entries.size()) return nullptr;

    const auto& src = arc->entries[idx].content;
    if (out_size) *out_size = src.size();

    u8* buf = new u8[src.size()];
    memcpy(buf, src.data(), src.size());
    return buf;
}

// Define vtable *after* functions exist
static ArchiveBaseVTable g_baseVTable = {
    &GetEntryCount,
    &GetEntryName,
    &GetEntrySize,
    &OpenStream
};

// -------------------- Archive Format Wrapper --------------------
static ArchiveHandle Example_New(sdk_ctx* /*ctx*/) {
    return reinterpret_cast<ArchiveHandle>(1); // dummy handle
}

static int Example_CanHandleFile(ArchiveHandle /*inst*/, u8* /*buffer*/, u64 /*size*/, const char* ext) {
    return ext && strcmp(ext, "example") == 0;
}

static ArchiveBaseHandle Example_TryOpen(ArchiveHandle /*inst*/, u8* buffer, u64 size, const char* filename) {
    auto* arc = new DemoArchive();

    arc->entries.push_back({
        "hello.txt",
        std::vector<u8>{'H','e','l','l','o','\n'}
    });

    ArchiveBaseHandle h{};
    h.inst = arc;
    h.vtable = &g_baseVTable;
    return h;
}

static const char* Example_GetTag(ArchiveHandle /*inst*/) {
    return "ExampleArchive";
}

static const char* Example_GetDescription(ArchiveHandle /*inst*/) {
    return "Example Archive!";
}

// -------------------- Global State --------------------
static ArchiveFormatVTable g_formatVTable;

// -------------------- Plugin Interface --------------------
extern "C" {

RD_EXPORT bool RD_PluginInit(HostAPI* api) {
    if (!api) return false;
    sdk_ctx* ctx = api->get_sdk_context();
    if (api->log && ctx) api->log(ctx, "Example plugin initialized");

    g_formatVTable = {
        .New = Example_New,
        .CanHandleFile = Example_CanHandleFile,
        .TryOpen = Example_TryOpen,
        .GetTag = Example_GetTag,
        .GetDescription = Example_GetDescription,
    };

    return true;
}

RD_EXPORT void RD_PluginShutdown() {
    // no global cleanup needed
}

RD_EXPORT const ArchiveFormatVTable* RD_GetArchiveFormat(sdk_ctx* /*ctx*/) {
    return &g_formatVTable;
}

} // extern "C"
