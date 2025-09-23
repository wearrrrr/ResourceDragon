#pragma once

#if defined(CPPTRACE_ENABLED)
    #include <cpptrace/cpptrace.hpp>
#else
#ifdef EMSCRIPTEN
    #include <emscripten.h>
    #include <stdio.h>
#else
    #include <stdio.h>
#endif
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

    #ifdef EMSCRIPTEN
    inline static std::string generate_stacktrace() {
        char callstack[4096];
        emscripten_get_callstack(EM_LOG_JS_STACK, callstack, sizeof(callstack));

        return std::string(callstack);
    }
    inline static void print_stacktrace() {
        std::puts(generate_stacktrace().c_str());
    }
    #else
        inline static std::string generate_stacktrace() {
            char buffer[4096];

            sprintf(buffer, "Unable to generate stacktrace! You are likely running from an unsupported platform\n");
            sprintf(buffer, "Platform Info:\n \t%s-%s \n\t%s-%s\n", CURRENT_PLATFORM, CURRENT_ARCH, CURRENT_COMPILER, CURRENT_COMPILER_VERSION);

            return std::string(buffer);
        }

        static void print_stacktrace() {
            std::puts(generate_stacktrace().c_str());
        }
    #endif


    #endif
}
