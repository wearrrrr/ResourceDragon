#include "example_plugin.h"
#include "../SDK/util/Logger.hpp"
#include "../SDK/util/rd_log_helpers.h"
#include "../SDK/util/rd_log.h"
#include <cstring>
#include <vector>

// Aligns with variables marked as RD_EXPORT in example_plugin.h
const char* RD_PluginName = "Demo Plugin";
const char* RD_PluginVersion = "1.0.0";
sdk_ctx* g_ctx = nullptr;

// -------------------- Base vtable functions --------------------
static usize GetEntryCount(ArchiveInstance inst) {
    auto* arc = (DemoArchive*)inst;
    return arc ? arc->entries.size() : 0;
}

static const char* GetEntryName(ArchiveInstance inst, usize idx) {
    auto* arc = (DemoArchive*)inst;
    if (!arc || idx >= arc->entries.size()) return nullptr;
    return arc->entries[idx].name.c_str();
}

static usize GetEntrySize(ArchiveInstance inst, usize idx) {
    auto* arc = (DemoArchive*)inst;
    if (!arc || idx >= arc->entries.size()) return 0;
    return arc->entries[idx].content.size();
}

static u8* OpenStream(ArchiveInstance inst, usize idx, usize* out_size) {
    auto* arc = (DemoArchive*)inst;
    if (!arc || idx >= arc->entries.size()) return nullptr;

    const auto& src = arc->entries[idx].content;
    if (out_size) *out_size = src.size();

    u8* buf = (u8*)malloc(src.size());
    memcpy(buf, src.data(), src.size());
    return buf;
}

static void ArchiveDestroy(ArchiveBaseHandle *handle) {
    rd_log(RD_LOG_LVL_INFO, "ArchiveDestroy called!");
    delete handle;
}

static ArchiveBaseVTable g_baseVTable = {
    &GetEntryCount,
    &GetEntryName,
    &GetEntrySize,
    &OpenStream,
    &ArchiveDestroy
};

// -------------------- Archive Format Wrapper --------------------
static ArchiveHandle Example_New(sdk_ctx* /*ctx*/) {
    return new DemoArchive();
}

static int Example_CanHandleFile(ArchiveHandle /*inst*/, u8* /*buffer*/, u64 /*size*/, const char* ext) {
    return ext && strcmp(ext, "example") == 0;
}

static ArchiveBaseHandle* Example_TryOpen(ArchiveHandle /*inst*/, u8* buffer, u64 size, const char* filename) {
    auto* arc = new DemoArchive();

    arc->entries.push_back({
        "hello.txt",
        std::vector<u8>{'H','e','l','l','o','\n'}
    });

    ArchiveBaseHandle *h = new ArchiveBaseHandle();
    h->inst = arc;
    h->vtable = &g_baseVTable;

    Logger::log("Example plugin opened '{}' ({} bytes, {} entries)", filename, size, arc->entries.size());

    return h;
}

static const char* Example_GetTag(ArchiveHandle /*inst*/) {
    return "DemoArchive";
}

static const char* Example_GetDescription(ArchiveHandle /*inst*/) {
    return "Demo Plugin for ResourceDragon using native code.";
}

static ArchiveFormatVTable g_formatTable = {
    .New = Example_New,
    .CanHandleFile = Example_CanHandleFile,
    .TryOpen = Example_TryOpen,
    .GetTag = Example_GetTag,
    .GetDescription = Example_GetDescription,
};

// -------------------- Plugin Interface --------------------
extern "C" {

bool RD_PluginInit(HostAPI *api) {
    if (!api) return false;
    sdk_ctx* ctx = api->get_sdk_context();
    Logger::log("Example plugin initialized");
    g_ctx = ctx;

    return true;
}

void RD_PluginShutdown() {
    // no global cleanup needed
}

const ArchiveFormatVTable* RD_GetArchiveFormat(sdk_ctx*) {
    return &g_formatTable;
}

} // extern "C"
