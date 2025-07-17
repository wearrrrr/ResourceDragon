#pragma once

#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <memory>
#include <typeinfo>
#include <cxxabi.h>

#define PREFIX "[ResourceDragon] "
#define RESET  "\x1B[0;1m"

#define LOG(format, ...) va_list args; va_start(args, format); vprintf(format, args); va_end(args);

class Logger {
    public:

    static void log(const char *format, ...) {
        printf("\x1b[1;34m%s", PREFIX RESET);
        LOG(format);
        puts(RESET);
    }

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
          "\x1b[1;34m" PREFIX RESET
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


    static void warn(const char *format, ...) {
        printf("\x1b[1;33m%s", PREFIX RESET);
        LOG(format);
        puts(RESET);
    }

    static void error(const char *format, ...) {
        printf("\x1b[1;31m%s", PREFIX RESET);
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
