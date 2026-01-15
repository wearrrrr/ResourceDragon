#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/*
 * rd_log_helpers.h
 *
 * convenience layer for plugin authors who use the pure C ABI.
 * - Provides an argument span struct so you can pass (pointer, count) pairs easily.
 * - Supplies macros that build the temporary RD_LogArg array, compute its length,
 *   and forwards everything to rd_log_fmtv for you.
 *
 * This header is independent of C++ templates, so plain C or other FFI-based
 * plugins (Rust, Zig, etc.) can include it directly.
 */

#include "rd_log.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RD_LogArgSpan {
    const RD_LogArg* data;
    size_t count;
} RD_LogArgSpan;

/* Simple helper in case you want to pass spans around explicitly. */
static inline RD_LogArgSpan rd_log_make_span(const RD_LogArg* data, size_t count) {
    RD_LogArgSpan span;
    span.data = data;
    span.count = count;
    return span;
}

#ifdef __cplusplus
} // extern "C"
#endif

/*
 * C11 _Generic-based automatic type conversion
 *
 * Allows C code to use automatic type deduction similar to C++:
 *   RD_LOG_ARG(42)           -> automatically creates rd_log_make_s64(42)
 *   RD_LOG_ARG("string")     -> automatically creates rd_log_make_cstring("string")
 *   RD_LOG_ARG(size_var)     -> automatically creates rd_log_make_size(size_var)
 *
 * This makes C logging much cleaner while maintaining full type safety.
 * Note: This is C-only; C++ uses templates instead.
 */
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
/* C11 _Generic automatic type conversion
 * Note: size_t and ptrdiff_t are typedefs that resolve to primitive types,
 * so they're handled by their underlying type (usually unsigned long/long long)
 */
#define RD_LOG_ARG(x) _Generic((x), \
    char*: rd_log_make_cstring, \
    const char*: rd_log_make_cstring, \
    _Bool: rd_log_make_bool, \
    char: rd_log_make_s64, \
    signed char: rd_log_make_s64, \
    short: rd_log_make_s64, \
    int: rd_log_make_s64, \
    long: rd_log_make_s64, \
    long long: rd_log_make_s64, \
    unsigned char: rd_log_make_u64, \
    unsigned short: rd_log_make_u64, \
    unsigned int: rd_log_make_u64, \
    unsigned long: rd_log_make_u64, \
    unsigned long long: rd_log_make_u64, \
    float: rd_log_make_f64, \
    double: rd_log_make_f64, \
    void*: rd_log_make_ptr, \
    const void*: rd_log_make_ptr, \
    default: rd_log_make_ptr \
)(x)

#endif /* C11 && !__cplusplus */


/*
 * Logging macros (C/C++)
 *
 * Primary macros (recommended):
 *   RD_LOG_INFO("Opened '{}' with {} entries", arg1, arg2);
 *   RD_LOG_WARN("Warning: {}", arg);
 *   RD_LOG_ERROR("Error occurred");
 *   RD_LOG(RD_LOG_INFO, "Custom level: {}", arg);
 *
 * For C89/C99: Use rd_log_make_* helpers explicitly
 *   RD_LOG_INFO("Value: {}", rd_log_make_s64(123));
 *
 * For C11: Use RD_LOG_ARG() for automatic type conversion
 *   RD_LOG_INFO("Opened '{}' with {} entries", RD_LOG_ARG(name), RD_LOG_ARG(size));
 *
 * For C++: Arguments are automatically converted via templates, use Logger class instead.
 *
 * These macros handle zero arguments gracefully and allocate a temporary
 * RD_LogArg array on the stack.
 */

/* General logging macro with custom level */
#define RD_LOG(level, fmt, ...) \
    do { \
        RD_LogArg _rd_log_args[] = { __VA_ARGS__ }; \
        rd_log_fmtv((level), (fmt), \
            sizeof(_rd_log_args) > 0 ? _rd_log_args : NULL, \
            sizeof(_rd_log_args) / (sizeof(_rd_log_args) > 0 ? sizeof(_rd_log_args[0]) : 1)); \
    } while (0)

/* Level-specific macros */
#define RD_LOG_INFO(fmt, ...) \
    do { \
        RD_LogArg _rd_log_args[] = { __VA_ARGS__ }; \
        rd_log_fmtv(RD_LOG_LVL_INFO, (fmt), \
            sizeof(_rd_log_args) > 0 ? _rd_log_args : NULL, \
            sizeof(_rd_log_args) / (sizeof(_rd_log_args) > 0 ? sizeof(_rd_log_args[0]) : 1)); \
    } while (0)

#define RD_LOG_WARN(fmt, ...) \
    do { \
        RD_LogArg _rd_log_args[] = { __VA_ARGS__ }; \
        rd_log_fmtv(RD_LOG_LVL_WARN, (fmt), \
            sizeof(_rd_log_args) > 0 ? _rd_log_args : NULL, \
            sizeof(_rd_log_args) / (sizeof(_rd_log_args) > 0 ? sizeof(_rd_log_args[0]) : 1)); \
    } while (0)

#define RD_LOG_ERROR(fmt, ...) \
    do { \
        RD_LogArg _rd_log_args[] = { __VA_ARGS__ }; \
        rd_log_fmtv(RD_LOG_LVL_ERROR, (fmt), \
            sizeof(_rd_log_args) > 0 ? _rd_log_args : NULL, \
            sizeof(_rd_log_args) / (sizeof(_rd_log_args) > 0 ? sizeof(_rd_log_args[0]) : 1)); \
    } while (0)


