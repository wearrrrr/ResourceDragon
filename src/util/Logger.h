#pragma once

#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <memory>
#include <typeinfo>
#include <cxxabi.h>

#define LOG(format, ...) va_list args; va_start(args, format); vprintf(format, args); va_end(args);

class Logger {
    public:

    static constexpr const char* PREFIX = "[ResourceDragon] ";
    static constexpr const char* RESET  = "\x1B[0;1m";

    static void log(const char *format, ...) {
        printf("\x1b[1;34m%s%s", PREFIX, RESET);
        LOG(format);
        puts(RESET);
    }

    template <typename T>
    static std::string get_type_name(const T& obj) {
        const char* mangled = typeid(obj).name();
        int status;
        std::unique_ptr<char, void(*)(void*)> demangled(
            abi::__cxa_demangle(mangled, nullptr, nullptr, &status),
            std::free
        );
        return (status == 0) ? demangled.get() : mangled;
    }

    template <typename T>
    static void log_struct(const T& obj) {
        printf("\x1b[1;34m%s%s", PREFIX, RESET);
        printf("Dumping struct %s\n", get_type_name(obj).data());
        #if defined(__clang__) && __has_builtin(__builtin_dump_struct)
        __builtin_dump_struct(&obj, printf);
        #else
        printf("__builtin_dump_struct is not supported with this compiler!");
        #endif
        puts(RESET);
    }


    static void warn(const char *format, ...) {
        printf("\x1b[1;33m%s%s", PREFIX, RESET);
        LOG(format);
        puts(RESET);
    }

    static void error(const char *format, ...) {
        printf("\x1b[1;31m%s%s", PREFIX, RESET);
        LOG(format);
        puts(RESET);
    }

    static void log(const std::string &str) {
        log("%s", str.c_str());
    }
    static void error(const std::string &str) {
        error("%s", str.c_str());
    }
};
