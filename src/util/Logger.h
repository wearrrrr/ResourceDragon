#pragma once

#include <string>
#include <stdio.h>
#include <stdarg.h>

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
    static void warn(const std::string &str) {
        warn("%s", str.c_str());
    }
    static void error(const std::string &str) {
        error("%s", str.c_str());
    }
};