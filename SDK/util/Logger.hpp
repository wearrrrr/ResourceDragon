#pragma once

#ifndef __cplusplus
#error "Logger.hpp is a C++ header and cannot be used from C. Use rd_log.h and rd_log_helpers.h instead."
#endif

#include "rd_log.h"
#include "rd_log_schema.h"
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <typeinfo>
#include <vector>

namespace rd_log_cpp_detail {

#if defined(__cpp_consteval) && __cpp_consteval >= 201811L
/* Compile-time format string validation for C++20+
 * Note: This can only validate string literals, not runtime strings.
 * Use it manually if needed: validate_format_braces("my format {}")
 */
consteval bool validate_format_braces(const char* fmt) {
    int depth = 0;
    for (const char* p = fmt; *p; ++p) {
        if (*p == '{') {
            if (*(p + 1) == '{') { ++p; continue; }  // escaped
            ++depth;
        } else if (*p == '}') {
            if (*(p + 1) == '}') { ++p; continue; }  // escaped
            --depth;
        }
        if (depth < 0) return false;  // unmatched closing brace
    }
    return depth == 0;  // all braces matched
}
#endif

inline RD_LogArg make_arg_direct(const RD_LogArg& arg) { return arg; }
inline RD_LogArg make_arg_direct(const char* str) { return rd_log_make_cstring(str); }
inline RD_LogArg make_arg_direct(std::string_view sv) { return rd_log_make_string(sv.data(), sv.size()); }
inline RD_LogArg make_arg_direct(const std::string& s) { return rd_log_make_string(s.data(), s.size()); }
inline RD_LogArg make_arg_direct(bool value) { return rd_log_make_bool(value ? 1u : 0u); }

template <typename T>
struct is_true_integral
    : std::bool_constant<std::is_integral_v<std::remove_reference_t<T>> && !std::is_same_v<std::remove_reference_t<T>, bool>> {};

template <typename T>
inline constexpr bool is_true_integral_v = is_true_integral<T>::value;

template <typename T>
inline std::enable_if_t<
    is_true_integral_v<std::remove_reference_t<T>> &&
    std::is_signed_v<std::remove_reference_t<T>>,
    RD_LogArg
>
make_arg_direct(T value) {
    return rd_log_make_s64(static_cast<long long>(value));
}

template <typename T>
inline std::enable_if_t<
    is_true_integral_v<std::remove_reference_t<T>> &&
    std::is_unsigned_v<std::remove_reference_t<T>>,
    RD_LogArg>
make_arg_direct(T value) {
    return rd_log_make_u64(static_cast<unsigned long long>(value));
}

template <typename T>
inline std::enable_if_t<std::is_floating_point_v<std::remove_reference_t<T>>, RD_LogArg>
make_arg_direct(T value) {
    return rd_log_make_f64(static_cast<double>(value));
}

template <typename T, typename = void>
struct has_direct_arg : std::false_type {};

template <typename T>
struct has_direct_arg<T, std::void_t<decltype(make_arg_direct(std::declval<T>()))>> : std::true_type {};

template <typename T, typename = void>
struct has_to_string : std::false_type {};

template <typename T>
struct has_to_string<T, std::void_t<decltype(std::declval<const T&>().to_string())>> : std::true_type {};

template <typename T>
struct is_std_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T, typename = void>
struct is_stream_insertable : std::false_type {};

template <typename T>
struct is_stream_insertable<T, std::void_t<
    decltype(std::declval<std::ostream&>() << std::declval<const std::remove_cvref_t<T>&>())
>> : std::true_type {};

template <typename T>
inline constexpr bool has_direct_arg_v = has_direct_arg<T>::value;

inline std::string quote_string(std::string_view value) {
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('"');
    result.append(value.begin(), value.end());
    result.push_back('"');
    return result;
}

template <typename T>
inline std::string stringify_fallback(const T& value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (is_std_vector<U>::value) {
        std::string result = "{ ";
        for (size_t i = 0; i < value.size(); ++i) {
            result += stringify_fallback(value[i]);
            if (i + 1 < value.size()) {
                result += ", ";
            }
        }
        result += " }";
        return result;
    } else if constexpr (std::is_same_v<U, std::string> || std::is_same_v<U, std::string_view>) {
        return quote_string(std::string_view(value));
    } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
        return value ? quote_string(value) : "\"\"";
    } else if constexpr (has_to_string<U>::value) {
        return value.to_string();
    } else {
        return typeid(U).name();
    }
}

template <typename T>
inline RD_LogArg make_arg_with_storage(T&& value, std::vector<std::string>& scratch) {
    using U = std::remove_cvref_t<T>;
    if constexpr (has_direct_arg_v<U>) {
        return make_arg_direct(std::forward<T>(value));
    } else {
        scratch.emplace_back(stringify_fallback(value));
        const std::string& stored = scratch.back();
        return rd_log_make_string(stored.data(), stored.size());
    }
}

template <typename... Args>
inline void rd_log_format_cpp(RD_LogLevel level, const char* fmt, Args&&... args) {
    // Note: Format validation can only work with string literals.
    // For runtime strings, validation is skipped.
    if constexpr (sizeof...(args) == 0) {
        rd_log_fmtv(level, fmt, nullptr, 0);
    } else {
        std::vector<std::string> scratch;
        // Only reserve space for types that need string conversion
        constexpr size_t non_direct = ((has_direct_arg_v<std::remove_cvref_t<Args>> ? 0 : 1) + ...);
        if constexpr (non_direct > 0) {
            scratch.reserve(non_direct);
        }
        RD_LogArg packed_args[] = { make_arg_with_storage(std::forward<Args>(args), scratch)... };
        rd_log_fmtv(level, fmt, packed_args, sizeof...(args));
    }
}
} // namespace rd_log_cpp_detail

struct Logger {
    static void log(const char* msg) {
        rd_log(RD_LOG_LVL_INFO, msg, msg ? strlen(msg) : 0);
    }

    static void log(std::string_view s) {
        rd_log(RD_LOG_LVL_INFO, s.data(), s.size());
    }

    template <typename Fn>
    static void log(Fn&& fn) {
        std::forward<Fn>(fn)();
    }

    template <typename... Args>
    static std::enable_if_t<(sizeof...(Args) > 0), void> log(const char* fmt, Args&&... args) {
        rd_log_cpp_detail::rd_log_format_cpp(RD_LOG_LVL_INFO, fmt, std::forward<Args>(args)...);
    }

    static void warn(const char* msg) {
        rd_log(RD_LOG_LVL_WARN, msg, msg ? strlen(msg) : 0);
    }

    static void warn(std::string_view s) {
        rd_log(RD_LOG_LVL_WARN, s.data(), s.size());
    }

    template <typename... Args>
    static std::enable_if_t<(sizeof...(Args) > 0), void> warn(const char* fmt, Args&&... args) {
        rd_log_cpp_detail::rd_log_format_cpp(RD_LOG_LVL_WARN, fmt, std::forward<Args>(args)...);
    }

    static void error(const char* msg) {
        rd_log(RD_LOG_LVL_ERROR, msg, msg ? strlen(msg) : 0);
    }

    static void error(std::string_view s) {
        rd_log(RD_LOG_LVL_ERROR, s.data(), s.size());
    }

    template <typename... Args>
    static std::enable_if_t<(sizeof...(Args) > 0), void> error(const char* fmt, Args&&... args) {
        rd_log_cpp_detail::rd_log_format_cpp(RD_LOG_LVL_ERROR, fmt, std::forward<Args>(args)...);
    }

    static void log_schema(std::string_view schema, const RD_LogField* fields, size_t count) {
        rd_log_schema(
            RD_LOG_LVL_INFO,
            schema.data(),
            fields,
            count
        );
    }
};
