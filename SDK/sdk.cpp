#include "sdk.h"
#include "ArchiveFormatWrapper.h"
#include <SDK/util/Logger.hpp>

ArchiveFormatWrapper* AddArchiveFormat(struct sdk_ctx* ctx, const ArchiveFormatVTable* vtable) {
    if (!ctx || !vtable) {
        Logger::error("No valid context or vtable provided!");
        return nullptr;
    };

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
    if (!ctx) return;

    ctx->version = 1;
    ctx->logger = new Logger();
    ctx->archiveFormat = nullptr;

    if (ctx->logger) {
        ctx->logger->log("SDK initialized");
    }
}

void sdk_deinit(struct sdk_ctx* ctx) {
    if (!ctx) return;

    if (ctx->logger) {
        ctx->logger->log("SDK shutting down");
        delete ctx->logger;
        ctx->logger = nullptr;
    }

    if (ctx->archiveFormat) {
        delete ctx->archiveFormat;
        ctx->archiveFormat = nullptr;
    }
}

} // extern "C"
