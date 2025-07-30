#pragma once

#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <memory>
#include <typeinfo>
#include <cxxabi.h>

#define PREFIX "[ResourceDragon] "
#define LOG_COLOR "\x1b[1;34m"
#define WARN_COLOR "\x1b[1;33m"
#define ERROR_COLOR "\x1b[1;31m"
#define RESET  "\x1B[0;1m"

#define LOG_PREFIX      LOG_COLOR PREFIX RESET
#define WARN_PREFIX     WARN_COLOR PREFIX RESET
#define ERROR_PREFIX    ERROR_COLOR PREFIX RESET

#define VA_LOG(func) va_list va; va_start(va, format); func(format, va); va_end(va)

struct Logger {
    template <typename T>
    static std::string get_type_name(const T& obj) {
        const char* mangled = typeid(obj).name();
        int status = 0;
        std::unique_ptr<char[], decltype(&std::free)> demangled(
            abi::__cxa_demangle(mangled, nullptr, nullptr, &status),
            &std::free
        );
        return (status == 0 && demangled) ? demangled.get() : mangled;
    }

    template <typename T>
    static void log_struct(const T& obj) {
        printf(
          LOG_PREFIX
          "Dumping struct %s\n",
          get_type_name(obj).data()
        );
#if defined(__clang__) && __has_builtin(__builtin_dump_struct)
        __builtin_dump_struct(&obj, printf);
#else
        printf("__builtin_dump_struct is not supported with this compiler!");
#endif
        puts(RESET);
    }

    static void log(const char* format, va_list va) {
        printf(LOG_PREFIX);
        vprintf(format, va);
        puts(RESET);
    }
    static void warn(const char* format, va_list va) {
        printf(WARN_PREFIX);
        vprintf(format, va);
        puts(RESET);
    }
    static void error(const char* format, va_list va) {
        printf(ERROR_PREFIX);
        vprintf(format, va);
        puts(RESET);
    }

    // These are templates purely to
    // prioritize the non-variadic version
    template <typename T = void>
    static void log(const char* format, ...) {
        VA_LOG(log);
    }
    template <typename T = void>
    static void warn(const char* format, ...) {
        VA_LOG(warn);
    }
    template <typename T = void>
    static void error(const char* format, ...) {
        VA_LOG(error);
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

    template <typename L>
    static void log(const L& lambda) {
        printf(LOG_PREFIX);
        lambda();
        puts(RESET);
    }
    template <typename L>
    static void warn(const L& lambda) {
        printf(WARN_PREFIX);
        lambda();
        puts(RESET);
    }
    template <typename L>
    static void error(const L& lambda) {
        printf(ERROR_PREFIX);
        lambda();
        puts(RESET);
    }
};
