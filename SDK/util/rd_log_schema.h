#pragma once
#include "rd_log.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RD_LogField {
    const char* name;
    const void* data;
    size_t size;
} RD_LogField;

/*
 * Emit structured log data.
 * Host decides how to interpret fields.
 */
void rd_log_schema(
    RD_LogLevel level,
    const char* schema_name,
    const RD_LogField* fields,
    size_t field_count
);

/* Helper macros for building RD_LogField structs */
#define RD_LOG_FIELD_STR(name, str) \
    { #name, (str), (str) ? strlen(str) : 0 }

#define RD_LOG_FIELD_CSTR(name, cstr) \
    { name, (cstr), (cstr) ? strlen(cstr) : 0 }

#define RD_LOG_FIELD_DATA(name, ptr, sz) \
    { name, (ptr), (sz) }

#define RD_LOG_FIELD_INT(name, val) \
    { name, &(val), sizeof(val) }

/* Emit structured log with automatic field count */
#define RD_LOG_SCHEMA(level, schema_name, ...) \
    do { \
        RD_LogField _rd_schema_fields[] = { __VA_ARGS__ }; \
        rd_log_schema((level), (schema_name), _rd_schema_fields, \
            sizeof(_rd_schema_fields) / sizeof(_rd_schema_fields[0])); \
    } while (0)

#ifdef __cplusplus
}

/* C++ template wrapper for compile-time array size deduction */
namespace rd_log_cpp_detail {
    template<size_t N>
    inline void log_schema_array(RD_LogLevel level, const char* schema, const RD_LogField (&fields)[N]) {
        rd_log_schema(level, schema, fields, N);
    }
}

#endif
