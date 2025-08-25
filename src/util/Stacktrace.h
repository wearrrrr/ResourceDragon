#pragma once


#if defined(CPPTRACE_ENABLED)
    #include <cpptrace/cpptrace.hpp>
#endif

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define HAS_CXXABI
#endif

#include <util/platform.h>
#include <util/int.h>

namespace Stacktrace {
    #ifdef CPPTRACE_ENABLED
    static void print_stacktrace() {
        cpptrace::generate_trace().print();
    }
    #else
    #include <stdio.h>
    static void print_stacktrace() {
        printf("Unable to generate stacktrace! You are likely running from an unsupported platform\n");
        printf("Platform Info:\n \t%s-%s \n\t%s-%s\n", CURRENT_PLATFORM, CURRENT_ARCH, CURRENT_COMPILER, CURRENT_COMPILER_VERSION);
        printf("Debug mode enabled? %s\n", DEBUG ? "Yes" : "No");
    }
    #endif
}
