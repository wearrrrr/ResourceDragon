#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RD_LogLevel {
    RD_LOG_LVL_INFO,
    RD_LOG_LVL_WARN,
    RD_LOG_LVL_ERROR,
    /* Compatibility aliases */
    RD_LOG_INFO = RD_LOG_LVL_INFO,
    RD_LOG_WARN = RD_LOG_LVL_WARN,
    RD_LOG_ERROR = RD_LOG_LVL_ERROR
} RD_LogLevel;

typedef enum RD_LogArgType {
    RD_LOG_ARG_STRING = 0,
    RD_LOG_ARG_BOOL,
    RD_LOG_ARG_S64,
    RD_LOG_ARG_U64,
    RD_LOG_ARG_F64
} RD_LogArgType;

typedef struct RD_LogStringArg {
    const char* data;
    size_t size;
} RD_LogStringArg;

typedef struct RD_LogArg {
    RD_LogArgType type;
    union {
        RD_LogStringArg string;
        uint8_t boolean;
        long long s64;
        unsigned long long u64;
        double f64;
    } value;
} RD_LogArg;

void rd_log(RD_LogLevel level, const char* msg, size_t len);

void rd_log_fmtv(RD_LogLevel level, const char* fmt, const RD_LogArg* args, size_t arg_count);

/* Convenience helpers for C ABI users */
static inline RD_LogArg rd_log_make_string(const char* data, size_t size) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_STRING;
    arg.value.string.data = data;
    arg.value.string.size = size;
    return arg;
}

static inline RD_LogArg rd_log_make_cstring(const char* data) {
    return rd_log_make_string(data, data ? strlen(data) : 0);
}

static inline RD_LogArg rd_log_make_bool(uint8_t value) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_BOOL;
    arg.value.boolean = value ? 1u : 0u;
    return arg;
}

static inline RD_LogArg rd_log_make_s64(long long value) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_S64;
    arg.value.s64 = value;
    return arg;
}

static inline RD_LogArg rd_log_make_u64(unsigned long long value) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_U64;
    arg.value.u64 = value;
    return arg;
}

static inline RD_LogArg rd_log_make_f64(double value) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_F64;
    arg.value.f64 = value;
    return arg;
}

/* Convenience helpers for common types */
static inline RD_LogArg rd_log_make_size(size_t value) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_U64;
    arg.value.u64 = (unsigned long long)value;
    return arg;
}

static inline RD_LogArg rd_log_make_ssize(ptrdiff_t value) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_S64;
    arg.value.s64 = (long long)value;
    return arg;
}

static inline RD_LogArg rd_log_make_ptr(const void* ptr) {
    RD_LogArg arg;
    arg.type = RD_LOG_ARG_U64;
    arg.value.u64 = (unsigned long long)(uintptr_t)ptr;
    return arg;
}

#ifdef __cplusplus
}
#endif
