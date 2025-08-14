#pragma once

#ifdef DEBUG
    #define USE_CPPTRACE
    #include <cpptrace/cpptrace.hpp>
#elif !defined(_WIN32)
    #define USE_EXECINFO
    #include <execinfo.h>
    #include <dlfcn.h>
#endif

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define HAS_CXXABI
#endif

#include <util/int.h>


namespace Stacktrace {
    #ifdef USE_CPPTRACE
    static void print_stacktrace() {
        cpptrace::generate_trace().print();
    }
    #elif defined(USE_EXECINFO)
    static void print_stacktrace() {
        void *frames[64];
        int count = backtrace(frames, 64);
        char **symbols = backtrace_symbols(frames, count);

        if (!symbols) return;

        for (int i = 0; i < count; i++) {
            Dl_info info;
            if (dladdr(frames[i], &info) && info.dli_sname) {
                // Demangle
                int status = 0;
                char *demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
                const char *name = (status == 0 && demangled) ? demangled : info.dli_sname;
                printf("%s (%p)\n", name, frames[i]);
                free(demangled);
            } else {
                printf("%s\n", symbols[i]);
            }
        }

        free(symbols);
    }
    #else
    static void print_stacktrace() {
        printf("Unable to generate stacktrace! You are likely running from an unsupported platform.\n");
    }
    #endif
}
