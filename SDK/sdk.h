#pragma once
#include "util/int.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sdk_ctx;

typedef void* ArchiveHandle;
typedef void* ArchiveInstance;

struct ArchiveBaseVTable {
    void (*Destroy)(ArchiveInstance inst);
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
    void (*Delete)(ArchiveHandle inst);

    int  (*CanHandleFile)(ArchiveHandle inst, u8* buffer, u64 size, const char* ext);
    ArchiveBaseHandle (*TryOpen)(ArchiveHandle inst, u8* buffer, u64 size, const char* file_name);

    const char *(*GetTag)(ArchiveHandle inst);
    const char *(*GetDescription)(ArchiveHandle inst);
} ArchiveFormatVTable;

// Plugin should export this
const ArchiveFormatVTable* RD_GetArchiveFormat(struct sdk_ctx* ctx);

void sdk_init(struct sdk_ctx* ctx);
void sdk_deinit(struct sdk_ctx* ctx);

void Logger_log(struct sdk_ctx* ctx, const char *msg, ...);
void Logger_warn(struct sdk_ctx* ctx, const char *fmt, ...);
void Logger_error(struct sdk_ctx* ctx, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
