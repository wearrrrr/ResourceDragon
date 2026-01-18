#pragma once
#include "rd_log.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RD_LogField {
    const char* name;
    const void* data;
    size_t size;
    uint32_t flags;
} RD_LogField;

typedef enum RD_LogFieldFlags {
    RD_LOG_FIELD_FLAG_NONE     = 0,
    RD_LOG_FIELD_FLAG_STRING   = 1u << 0,
    RD_LOG_FIELD_FLAG_BINARY   = 1u << 1,
    RD_LOG_FIELD_FLAG_INTEGER  = 1u << 2,
    RD_LOG_FIELD_FLAG_UNSIGNED = 1u << 3,
    RD_LOG_FIELD_FLAG_FLOAT    = 1u << 4,
    RD_LOG_FIELD_FLAG_BOOLEAN  = 1u << 5
} RD_LogFieldFlags;

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
#define RD_LOG_FIELD_EX(field_name, ptr, sz, field_flags) \
    { (field_name), (ptr), (sz), (field_flags) }

#define RD_LOG_FIELD_STR(name, str) \
    RD_LOG_FIELD_EX(#name, (str), (str) ? strlen(str) : 0, RD_LOG_FIELD_FLAG_STRING)

#define RD_LOG_FIELD_CSTR(name, cstr) \
    RD_LOG_FIELD_EX((name), (cstr), (cstr) ? strlen(cstr) : 0, RD_LOG_FIELD_FLAG_STRING)

#define RD_LOG_FIELD_DATA(name, ptr, sz) \
    RD_LOG_FIELD_EX((name), (ptr), (sz), RD_LOG_FIELD_FLAG_BINARY)

#define RD_LOG_FIELD_INT(name, val) \
    RD_LOG_FIELD_EX((name), &(val), sizeof(val), RD_LOG_FIELD_FLAG_INTEGER)

#define RD_LOG_FIELD_U64(name, val) \
    RD_LOG_FIELD_EX((name), &(val), sizeof(val), RD_LOG_FIELD_FLAG_INTEGER | RD_LOG_FIELD_FLAG_UNSIGNED)

#define RD_LOG_FIELD_UINT(name, val) \
    RD_LOG_FIELD_U64((name), (val))

#define RD_LOG_FIELD_BOOL(name, val) \
    RD_LOG_FIELD_EX((name), &(val), sizeof(val), RD_LOG_FIELD_FLAG_BOOLEAN)

#define RD_LOG_FIELD_F64(name, val) \
    RD_LOG_FIELD_EX((name), &(val), sizeof(val), RD_LOG_FIELD_FLAG_FLOAT)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define RD_LOG_FIELD(name, value) \
    _Generic((value), \
        const char*: RD_LOG_FIELD_CSTR(name, value), \
        char*: RD_LOG_FIELD_CSTR(name, value), \
        bool: RD_LOG_FIELD_BOOL(name, value), \
        signed char: RD_LOG_FIELD_INT(name, value), \
        unsigned char: RD_LOG_FIELD_U64(name, value), \
        short: RD_LOG_FIELD_INT(name, value), \
        unsigned short: RD_LOG_FIELD_U64(name, value), \
        int: RD_LOG_FIELD_INT(name, value), \
        unsigned int: RD_LOG_FIELD_U64(name, value), \
        long: RD_LOG_FIELD_INT(name, value), \
        unsigned long: RD_LOG_FIELD_U64(name, value), \
        long long: RD_LOG_FIELD_INT(name, value), \
        unsigned long long: RD_LOG_FIELD_U64(name, value), \
        float: RD_LOG_FIELD_F64(name, value), \
        double: RD_LOG_FIELD_F64(name, value), \
        default: RD_LOG_FIELD_DATA((name), &(value), sizeof(value)) \
    )
#endif

/* Emit structured log with automatic field count */
#define RD_LOG_SCHEMA(level, schema_name, ...) \
    do { \
        RD_LogField _rd_schema_fields[] = { __VA_ARGS__ }; \
        rd_log_schema((level), (schema_name), _rd_schema_fields, \
            sizeof(_rd_schema_fields) / sizeof(_rd_schema_fields[0])); \
    } while (0)

#ifdef __cplusplus
}
#include <type_traits>
#include <string>
#include <string_view>
#include <utility>
#include <cstring>

/* C++ template helpers for schema logging */
namespace rd_log_cpp_detail {

    template<typename T>
    inline constexpr uint32_t deduce_field_flags() {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (std::is_same_v<U, bool>) {
            return RD_LOG_FIELD_FLAG_BOOLEAN;
        } else if constexpr (std::is_integral_v<U>) {
            uint32_t f = RD_LOG_FIELD_FLAG_INTEGER;
            if constexpr (std::is_unsigned_v<U>) {
                f |= RD_LOG_FIELD_FLAG_UNSIGNED;
            }
            return f;
        } else if constexpr (std::is_floating_point_v<U>) {
            return RD_LOG_FIELD_FLAG_FLOAT;
        } else {
            return RD_LOG_FIELD_FLAG_BINARY;
        }
    }

    inline RD_LogField make_string_field(const char* name, const char* data, size_t size) {
        RD_LogField field{};
        field.name = name;
        field.data = data;
        field.size = size;
        field.flags = RD_LOG_FIELD_FLAG_STRING;
        return field;
    }

    template<typename T>
    inline RD_LogField make_field(const char* name, const T& value) {
        RD_LogField field{};
        field.name = name;
        field.data = &value;
        field.size = sizeof(T);
        field.flags = deduce_field_flags<T>();
        return field;
    }

    inline RD_LogField make_field(const char* name, const char* value) {
        return make_string_field(name, value, value ? std::strlen(value) : 0);
    }

    inline RD_LogField make_field(const char* name, const std::string& value) {
        return make_string_field(name, value.data(), value.size());
    }

    inline RD_LogField make_field(const char* name, std::string_view value) {
        return make_string_field(name, value.data(), value.size());
    }

    template<size_t N>
    inline RD_LogField make_field(const char* name, const char (&value)[N]) {
        size_t len = std::char_traits<char>::length(value);
        return make_string_field(name, value, len);
    }

    template<typename Name, typename Value, typename... Rest>
    inline void append_schema_fields(RD_LogField* dst, Name&& name, Value&& value, Rest&&... rest) {
        static_assert(std::is_convertible_v<Name, const char*>, "Schema field names must be convertible to const char*");
        dst[0] = make_field(static_cast<const char*>(std::forward<Name>(name)), std::forward<Value>(value));
        if constexpr (sizeof...(Rest) > 0) {
            append_schema_fields(dst + 1, std::forward<Rest>(rest)...);
        }
    }

    inline void append_schema_fields(RD_LogField*) {}

    template<size_t N>
    inline void log_schema_array(RD_LogLevel level, const char* schema, const RD_LogField (&fields)[N]) {
        rd_log_schema(level, schema, fields, N);
    }

    template<typename... Fields>
    inline void log_schema_variadic(RD_LogLevel level, const char* schema, Fields&&... fields) {
        RD_LogField field_array[] = { std::forward<Fields>(fields)... };
        rd_log_schema(level, schema, field_array, sizeof...(fields));
    }

    template<typename... Args>
    inline void log_schema_auto(RD_LogLevel level, const char* schema, Args&&... args) {
        static_assert(sizeof...(Args) % 2 == 0, "log_schema_auto expects name/value pairs");
        constexpr size_t field_count = sizeof...(Args) / 2;
        if constexpr (field_count == 0) {
            rd_log_schema(level, schema, nullptr, 0);
        } else {
            RD_LogField fields[field_count];
            append_schema_fields(fields, std::forward<Args>(args)...);
            rd_log_schema(level, schema, fields, field_count);
        }
    }
}

#endif
