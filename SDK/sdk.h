#pragma once

#include "util/Logger.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sdk_ctx {
    int version;
    Logger logger;
};

void sdk_init(struct sdk_ctx *ctx);
void sdk_deinit(struct sdk_ctx *ctx);

#ifdef __cplusplus
}
#endif
