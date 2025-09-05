#pragma once

#if defined(CPPTRACE_ENABLED)
    #include <cpptrace/cpptrace.hpp>
#endif

#include <util/platform.h>
#include <util/int.h>

namespace Stacktrace {
    #ifdef CPPTRACE_ENABLED
    inline static cpptrace::stacktrace generate_stacktrace() {
        return cpptrace::generate_trace();
    }
    static void print_stacktrace() {
        generate_stacktrace().print();
    }
    #else
    #include <stdio.h>
    static void print_stacktrace() {
        printf("Unable to generate stacktrace! You are likely running from an unsupported platform\n");
        printf("Platform Info:\n \t%s-%s \n\t%s-%s\n", CURRENT_PLATFORM, CURRENT_ARCH, CURRENT_COMPILER, CURRENT_COMPILER_VERSION);
    }
    #endif
}
