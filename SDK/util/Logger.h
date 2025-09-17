#pragma once

#include <string>
#include <stdarg.h>

struct Logger {
    template <typename T>
    static void log_struct(const T& obj);

    static void print_stacktrace(const char *message = nullptr);

    template <typename T>
    static void log(const T& value);

    template <typename T>
    static void warn(const T& value);
    template <typename T>
    static void error(const T& value);

    // Arbitrary types
    template <typename... Args>
    static void log(const std::string_view &fmt, Args&&... args);

    template <typename... Args>
    static void warn(const std::string_view &fmt, Args&&... args);

    template <typename... Args>
    static void error(const std::string_view &fmt, Args&&... args);

    static void log(const char* str);
    static void warn(const char* str);
    static void error(const char* str);

    static void log(const std::string& str);
    static void warn(const std::string& str);
    static void error(const std::string& str);
};
