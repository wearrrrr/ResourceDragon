#include "sdk.h"
#include "ArchiveFormatWrapper.h"
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
            ctx->logger->log(std::string("Adding archive format " + std::string(tag)).c_str());
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

void Logger_log(struct sdk_ctx* ctx, const char* msg) {
    if (ctx && ctx->logger) {
        ctx->logger->log(msg);
    }
}
void Logger_warn(struct sdk_ctx* ctx, const char* msg) {
    if (ctx && ctx->logger) {
        ctx->logger->warn(msg);
    }
}
void Logger_error(struct sdk_ctx* ctx, const char* msg) {
    if (ctx && ctx->logger) {
        ctx->logger->error(msg);
    }
}

} // extern "C"
