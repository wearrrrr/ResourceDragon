#pragma once

#include <cstdarg>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <memory>
#include <typeinfo>

#include <fmt/core.h>

#include <util/Stacktrace.h>

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define HAS_CXXABI
#endif

#define PREFIX "[ResourceDragon] "
#define LOG_COLOR "\x1b[1;34m"
#define WARN_COLOR "\x1b[1;33m"
#define ERROR_COLOR "\x1b[1;31m"
#define RESET  "\x1B[0;1m"

#define LOG_PREFIX      LOG_COLOR PREFIX RESET
#define WARN_PREFIX     WARN_COLOR PREFIX RESET
#define ERROR_PREFIX    ERROR_COLOR PREFIX RESET

template <typename T, typename = void>
struct is_formattable : std::false_type {};

template <typename T>
struct is_formattable<T, std::void_t<
    decltype(fmt::format(std::declval<std::string>(), std::declval<T>()))
>> : std::true_type {};

template <typename T>
inline constexpr bool is_formattable_v = is_formattable<T>::value;

template <typename T>
static std::string get_type_name(const T& obj) {
    const char* mangled = typeid(obj).name();
#ifdef HAS_CXXABI
    int status = 0;
    std::unique_ptr<char[], decltype(&free)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status),
        &free
    );
    return (status == 0 && demangled) ? std::string(demangled.get()) : std::string(mangled);
#else
    return std::string(mangled);
#endif
}

template<typename T>
static std::string cvt_to_string(T&& val) {
    using U = std::remove_cvref_t<T>;

    if constexpr (requires { val.to_string(); }) {
        return val.to_string();
    }
    else if constexpr (is_formattable_v<U>) {
        return fmt::format("{}", val);
    }
    else {
        return get_type_name(val);
    }
}

template <typename T, typename = void>
struct LogTrait {
    static void print(const T& value) {
        if constexpr (requires { value.to_string(); }) {
            printf("%s", value.to_string().data());
        } else if constexpr (requires { value(); }) {
            value();
        } else {
            printf("<%s - unprintable>", get_type_name<T>(value).data());
        }
    }
};

template <typename T>
struct LogTrait<T, std::enable_if_t<std::is_integral_v<T>>> {
    static void print(const T& value) {
        printf("%d", value);
    }
};

template <typename T>
struct LogTrait<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static void print(const T& value) {
        printf("%f", value);
    }
};

template <typename T>
struct LogTrait<T, std::enable_if_t<std::is_pointer_v<T>>> {
    static void print(const T& value) {
        printf("%p", value);
    }
};

template <typename T, typename U>
struct LogTrait<std::pair<T, U>> {
    static void print(const std::pair<T, U>& p) {
        printf("(%s, %s)", cvt_to_string(p.first).c_str(), cvt_to_string(p.second).c_str());
    }
};

struct Logger {
    template <typename T>
    static void log_struct(const T& obj) {
        printf(
          LOG_PREFIX
          "Dumping struct %s\n",
          get_type_name(obj).data()
        );
#if defined(__clang__) && __has_builtin(__builtin_dump_struct) && defined(HAS_CXXABI)
        __builtin_dump_struct(&obj, &printf);
        puts(RESET);
#else
        Logger::error("__builtin_dump_struct is not supported with this compiler or cxxabi is missing!");
#endif
    }

    static void print_stacktrace(const char *message = nullptr) {
        if (message) Logger::warn(message);
        Stacktrace::print_stacktrace();
    }

    template <typename T>
    static void log(const T& value) {
        printf(LOG_PREFIX);
        LogTrait<T>::print(value);
        puts(RESET);
    }

    template <typename T>
    static void warn(const T& value) {
        printf(WARN_PREFIX);
        LogTrait<T>::print(value);
        puts(RESET);
    }
    template <typename T>
    static void error(const T& value) {
        printf(ERROR_PREFIX);
        LogTrait<T>::print(value);
        puts(RESET);
    }

    // Arbitrary types
    template <typename... Args>
    static void log(const std::string_view &fmt, Args&&... args) {
        auto arg_strings = std::make_tuple(cvt_to_string(std::forward<Args>(args))...);
        auto str = std::apply(
            [&](auto&... s) {
                return fmt::vformat(fmt, fmt::make_format_args(s...));
            },
            arg_strings
        );

        printf(LOG_PREFIX "%s" RESET "\n", str.c_str());
    }

    template <typename... Args>
    static void warn(const std::string_view &fmt, Args&&... args) {
        auto arg_strings = std::make_tuple(cvt_to_string(std::forward<Args>(args))...);
        auto str = std::apply(
            [&](auto&... s) {
                return fmt::vformat(fmt, fmt::make_format_args(s...));
            },
            arg_strings
        );

        printf(WARN_PREFIX "%s" RESET "\n", str.c_str());
    }

    template <typename... Args>
    static void error(const std::string_view &fmt, Args&&... args) {
        auto arg_strings = std::make_tuple(cvt_to_string(std::forward<Args>(args))...);
        auto str = std::apply(
            [&](auto&... s) {
                return fmt::vformat(fmt, fmt::make_format_args(s...));
            },
            arg_strings
        );

        printf(ERROR_PREFIX "%s" RESET "\n", str.c_str());
    }

    static void log(const char* str) {
        printf(LOG_PREFIX "%s" RESET "\n", str);
    }
    static void warn(const char* str) {
        printf(WARN_PREFIX "%s" RESET "\n", str);
    }
    static void error(const char* str) {
        printf(ERROR_PREFIX "%s" RESET "\n", str);
    }

    static void log(const std::string& str) {
        log(str.data());
    }
    static void warn(const std::string& str) {
        warn(str.data());
    }
    static void error(const std::string& str) {
        error(str.data());
    }
};
