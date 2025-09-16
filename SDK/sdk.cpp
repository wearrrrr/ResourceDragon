#include "sdk.h"
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif

void sdk_init(struct sdk_ctx *ctx) {
    ctx->version = 1;
    ctx->logger = Logger();
    ctx->logger.log("Initializing SDK...");
}

void sdk_deinit(struct sdk_ctx *ctx) {
    printf("Deiniting SDK...");
}

#ifdef __cplusplus
}
#endif
