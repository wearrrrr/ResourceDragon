#pragma once

#ifdef __linux__
#define CURRENT_PLATFORM "Linux"
#elif defined(__APPLE__)
#define CURRENT_PLATFORM "MacOS"
#elif defined(_WIN32)
#define CURRENT_PLATFORM "Windows"
#elif defined(EMSCRIPTEN)
#define CURRENT_PLATFORM "Emscripten (Wasm)"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define CURRENT_ARCH "x86_64"
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define CURRENT_ARCH "x86_32"
#elif defined(EMSCRIPTEN)
#define CURRENT_ARCH "wasm"
#elif defined(__aarch64__) || defined(_M_ARM64)
#define CURRENT_ARCH "ARM64"
#elif defined(mips) || defined(__mips__) || defined(__mips)
#define CURRENT_ARCH "MIPS"
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
#define CURRENT_ARCH "PPC"
#elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
#define CURRENT_ARCH "PPC64"
#else
#define CURRENT_ARCH "Unknown"
#endif

#if defined(__clang__)
#define CURRENT_COMPILER "Clang"
#define CURRENT_COMPILER_VERSION __clang_version__
#elif defined(__GNUC__)
#define CURRENT_COMPILER "GCC"
#define CURRENT_COMPILER_VERSION __VERSION__
#elif defined(_MSC_VER)
#define CURRENT_COMPILER "MSVC"
#define CURRENT_COMPILER_VERSION _MSC_VER
#else
#define CURRENT_COMPILER "Unknown"
#define CURRENT_COMPILER_VERSION "Unknown"
#endif
