#pragma once

#include "util/int.h"

struct sdk_ctx;

#ifdef __cplusplus
extern "C" {
#endif

void sdk_init(struct sdk_ctx** ctx);
void sdk_deinit(struct sdk_ctx* ctx);

typedef void *ArchiveHandle;

typedef struct ArchiveFormatVTable {
    ArchiveHandle (*New)(struct sdk_ctx* ctx);
    void (*Delete)(ArchiveHandle inst);

    int  (*CanHandleFile)(ArchiveHandle inst, u8* buffer, u64 size, const char* ext);
    void *(*TryOpen)(ArchiveHandle inst, u8* buffer, u64 size, const char* file_name);

    const char *(*GetTag)(ArchiveHandle inst);
    const char *(*GetDescription)(ArchiveHandle inst);
} ArchiveFormatVTable;

const ArchiveFormatVTable* RD_GetArchiveFormat(struct sdk_ctx* ctx);

void Logger_log(struct sdk_ctx* ctx, const char* msg);
void Logger_warn(struct sdk_ctx* ctx, const char* msg);
void Logger_error(struct sdk_ctx* ctx, const char* msg);

#ifdef __cplusplus
}
#endif
