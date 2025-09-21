#include "sdk.h"
#include "ArchiveFormatWrapper.h"
#include <cstdarg>
#include <string_view>
#include <util/Logger.h>

struct sdk_ctx {
    int version;
    Logger* logger;
    ArchiveFormatWrapper* archiveFormat;
};

ArchiveFormatWrapper* AddArchiveFormat(struct sdk_ctx* ctx, const ArchiveFormatVTable* vtable) {
    if (!ctx || !vtable) return nullptr;

    ArchiveHandle inst = nullptr;
    if (vtable->New) {
        inst = vtable->New(ctx);
    }

    const char* tag = nullptr;
    if (vtable->GetTag) {
        tag = vtable->GetTag(inst);
    }

    if (ctx->logger) {
        if (tag) {
            ctx->logger->log("Adding archive format {}", tag);
        } else {
            ctx->logger->log("Adding archive format (no tag)");
        }
    }

    ArchiveFormatWrapper* wrapper = new ArchiveFormatWrapper(vtable, ctx, inst);
    ctx->archiveFormat = wrapper;
    return wrapper;
}

extern "C" {

void sdk_init(struct sdk_ctx* ctx) {
    ctx->logger = new Logger();
    ctx->logger->log("SDK initialized");
}

void sdk_deinit(struct sdk_ctx* ctx) {
    if (ctx) {
        ctx->logger->log("SDK shutting down");
    }
}

extern "C" void Logger_log(struct sdk_ctx* ctx, const char *fmt, ...) {
    if (ctx && ctx->logger) {
        va_list args;
        va_start(args, fmt);
        ctx->logger->va_log(fmt, args);
        va_end(args);
    }
}
extern "C" void Logger_warn(struct sdk_ctx* ctx, const char *fmt, ...) {
    if (ctx && ctx->logger) {
        va_list args;
        va_start(args, fmt);
        ctx->logger->va_warn(fmt, args);
        va_end(args);
    }
}
extern "C" void Logger_error(struct sdk_ctx* ctx, const char *fmt, ...) {
    if (ctx && ctx->logger) {
        va_list args;
        va_start(args, fmt);
        ctx->logger->va_error(fmt, args);
        va_end(args);
    }
}

} // extern "C"
