#pragma once

struct sdk_ctx;

#ifdef __cplusplus
extern "C" {
#endif

void sdk_init(struct sdk_ctx** ctx);
void sdk_deinit(struct sdk_ctx* ctx);

void sdk_log(struct sdk_ctx* ctx, const char* msg);

#ifdef __cplusplus
}
#endif
