#include "sdk.h"
#include <util/Logger.h>

struct sdk_ctx {
    int version;
    Logger* logger;
};

extern "C" {

void sdk_init(struct sdk_ctx** ctx) {
    *ctx = new sdk_ctx();
    (*ctx)->version = 1;
    (*ctx)->logger = new Logger();
    (*ctx)->logger->log("SDK initialized");
}

void sdk_deinit(struct sdk_ctx* ctx) {
    if (ctx) {
        ctx->logger->log("SDK shutting down");
        delete ctx->logger;
        delete ctx;
    }
}

void Logger_log(struct sdk_ctx* ctx, const char* msg) {
    if (ctx) {
        ctx->logger->log(msg);
    }
}
void Logger_warn(struct sdk_ctx* ctx, const char* msg) {
    if (ctx) {
        ctx->logger->warn(msg);
    }
}
void Logger_error(struct sdk_ctx* ctx, const char* msg) {
    if (ctx) {
        ctx->logger->error(msg);
    }
}

}
