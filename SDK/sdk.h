#pragma once
#include "util/int.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  define RD_API extern "C" __declspec(dllexport) __cdecl
#else
#  define RD_API extern "C"
#endif

struct sdk_ctx {
    int version;
    struct Logger* logger;
    class ArchiveFormatWrapper* archiveFormat;
};

#ifdef _WIN32
using LogFn_t = void (__cdecl *)(struct sdk_ctx* ctx, const char *msg);
#else
using LogFn_t = void (*)(struct sdk_ctx* ctx, const char* fmt);
#endif

struct HostAPI {
    sdk_ctx* (*get_sdk_context)();
    LogFn_t log;
    LogFn_t warn;
    LogFn_t error;
};

typedef void* ArchiveHandle;
typedef void* ArchiveInstance;

struct ArchiveBaseVTable {
    usize (*GetEntryCount)(ArchiveInstance inst);
    const char* (*GetEntryName)(ArchiveInstance inst, usize index);
    usize (*GetEntrySize)(ArchiveInstance inst, usize index);
    u8* (*OpenStream)(ArchiveInstance inst, usize index, usize* out_size);
};
typedef struct {
    ArchiveInstance inst;
    const ArchiveBaseVTable* vtable;
} ArchiveBaseHandle;

typedef struct ArchiveFormatVTable {
    ArchiveHandle (*New)(struct sdk_ctx* ctx);

    int  (*CanHandleFile)(ArchiveHandle inst, u8* buffer, u64 size, const char* ext);
    ArchiveBaseHandle (*TryOpen)(ArchiveHandle inst, u8* buffer, u64 size, const char* file_name);

    const char *(*GetTag)(ArchiveHandle inst);
    const char *(*GetDescription)(ArchiveHandle inst);
} ArchiveFormatVTable;

void sdk_init(struct sdk_ctx* ctx);
void sdk_deinit(struct sdk_ctx* ctx);

void Logger_log(struct sdk_ctx* ctx, const char *fmt, ...);
void Logger_warn(struct sdk_ctx* ctx, const char *fmt, ...);
void Logger_error(struct sdk_ctx* ctx, const char *fmt, ...);

class ArchiveFormatWrapper* AddArchiveFormat(struct sdk_ctx* ctx, const ArchiveFormatVTable* vtable);

#ifdef _WIN32
#define RD_EXPORT extern "C" __declspec(dllexport)
#else
#define RD_EXPORT extern "C"
#endif

typedef bool (*RD_PluginInit_t)(HostAPI* api);
typedef void (*RD_PluginShutdown_t)();
typedef const ArchiveFormatVTable* (*RD_GetArchiveFormat_t)(struct sdk_ctx* ctx);

#ifdef __cplusplus
}
#endif
