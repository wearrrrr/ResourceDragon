#pragma once

#ifndef __cplusplus
#error "Logger.hpp is a C++ header and cannot be used from C. Use rd_log.h and rd_log_helpers.h instead."
#endif

#include "rd_log.h"
#include "rd_log_schema.h"
#include <functional>
#include <string>
#include <string_view>
#include <cstring>
#include <type_traits>
#include <utility>
#include <typeinfo>
#include <vector>

namespace rd_log_cpp_detail {

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
struct has_to_string<T, std::void_t<decltype(std::declval<std::remove_reference_t<T>&>().to_string())>> : std::true_type {};

template <typename T, typename = void>
struct has_const_to_string : std::false_type {};

template <typename T>
struct has_const_to_string<T, std::void_t<decltype(std::declval<const std::remove_reference_t<T>&>().to_string())>> : std::true_type {};



template <typename T>
struct is_std_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type {};

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
inline std::string invoke_to_string(T&& value) {
    using RawT = std::remove_reference_t<T>;
    using DecayedT = std::remove_cv_t<RawT>;
    if constexpr (has_const_to_string<DecayedT>::value) {
        return std::forward<T>(value).to_string();
    } else if constexpr (has_to_string<DecayedT>::value) {
        if constexpr (!std::is_const_v<RawT> && std::is_lvalue_reference_v<T&&>) {
            return value.to_string();
        } else if constexpr (std::is_move_constructible_v<DecayedT>) {
            DecayedT temp(std::forward<T>(value));
            return temp.to_string();
        } else if constexpr (std::is_copy_constructible_v<DecayedT>) {
            DecayedT temp(value);
            return temp.to_string();
        }
    }
    return typeid(DecayedT).name();
}

template <typename T>
inline std::string stringify_fallback(T&& value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (is_std_vector<U>::value) {
        const auto& ref = value;
        std::string result = "{ ";
        for (size_t i = 0; i < ref.size(); ++i) {
            result += stringify_fallback(ref[i]);
            if (i + 1 < ref.size()) {
                result += ", ";
            }
        }
        result += " }";
        return result;
    } else if constexpr (std::is_same_v<U, std::string> || std::is_same_v<U, std::string_view>) {
        return quote_string(std::string_view(value));
    } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
        return value ? quote_string(value) : "\"\"";
    } else if constexpr (has_const_to_string<U>::value || has_to_string<U>::value) {
        return invoke_to_string(std::forward<T>(value));
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
        scratch.emplace_back(stringify_fallback(std::forward<T>(value)));
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

    template <typename T>
    static std::enable_if_t<
        !std::is_invocable_v<T> &&
        !std::is_convertible_v<T, const char*> &&
        !std::is_same_v<std::decay_t<T>, std::string_view>,
    void>
    log(T&& value) {
        rd_log_cpp_detail::rd_log_format_cpp(RD_LOG_LVL_INFO, "{}", std::forward<T>(value));
    }

    template <typename Fn>
    static std::enable_if_t<std::is_invocable_v<Fn>, void> log(Fn&& fn) {
        std::invoke(std::forward<Fn>(fn));
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

    template <typename Name, typename Value, typename... Rest>
    static std::enable_if_t<
        std::is_convertible_v<Name, const char*> &&
        (sizeof...(Rest) % 2 == 0),
    void> log_schema(std::string_view schema, Name&& name, Value&& value, Rest&&... rest) {
        rd_log_cpp_detail::log_schema_auto(
            RD_LOG_LVL_INFO,
            schema.data(),
            std::forward<Name>(name),
            std::forward<Value>(value),
            std::forward<Rest>(rest)...
        );
    }
};
