# Platform-specific checks
set(LUA_PLATFORM "Autodetect" CACHE STRING "Target platform (POSIX, Windows, Freestanding or Autodetect)")
set_property(CACHE LUA_PLATFORM PROPERTY STRINGS "POSIX" "Windows" "Freestanding" "Autodetect")

if (NOT LUA_PLATFORM STREQUAL "POSIX" AND
        NOT LUA_PLATFORM STREQUAL "Windows" AND
        NOT LUA_PLATFORM STREQUAL "Freestanding" AND
        NOT LUA_PLATFORM STREQUAL "Autodetect")
    message(FATAL_ERROR "Invalid platform ${LUA_PLATFORM}")
endif ()

set(LUA_PLATFORM_SUPPORT "Freestanding")
set(LUA_USE_POSIX FALSE)
set(LUA_USE_WINDOWS FALSE)
if (LUA_PLATFORM STREQUAL "Autodetect")
    if (UNIX)
        set(LUA_PLATFORM_SUPPORT "POSIX")
        set(LUA_USE_POSIX TRUE)
    elseif (WIN32)
        set(LUA_PLATFORM_SUPPORT "Windows")
        set(LUA_USE_WINDOWS TRUE)
    endif ()
else ()
    set(LUA_PLATFORM_SUPPORT ${LUA_PLATFORM})
endif ()


# DLL support
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
set(LUA_BUILD_AS_DLL FALSE)
if (BUILD_SHARED_LIBS AND LUA_USE_WINDOWS)
    set(LUA_BUILD_AS_DLL TRUE)
endif ()

include(CMakePushCheckState)
include(CheckSymbolExists)

# Check for dynamic library loading with dlopen
# CMake already knows the library to link with (if any), however since
# CMAKE_DL_LIBS might not be set if dlopen is in libc, check for the
# symbol
set(lua_library_loader_default OFF)

if (UNIX)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
    check_symbol_exists(dlopen "dlfcn.h" LUA_HAVE_DLOPEN)
    cmake_pop_check_state()
endif ()

if ((WIN32 AND BUILD_SHARED_LIBS) OR (UNIX AND BUILD_SHARED_LIBS AND LUA_HAVE_DLOPEN))
    set(lua_library_loader_default ON)
endif ()
option(LUA_LIBRARY_LOADER "Lua will use dlopen to load libraries (POSIX) or LoadLibrary (Windows)" ${lua_library_loader_default})

set(LUA_LIBRARY_LOADER_SUPPORT None)
set(LUA_USE_DLOPEN FALSE)
set(LUA_DL_DLL FALSE)
if (LUA_LIBRARY_LOADER)
    if (WIN32)
        set(LUA_LIBRARY_LOADER_SUPPORT "WIN32 LoadLibrary")
        set(LUA_DL_DLL TRUE)
    elseif (UNIX)
        if (LUA_HAVE_DLOPEN)
            set(LUA_LIBRARY_LOADER_SUPPORT "POSIX dlopen")
            set(LUA_USE_DLOPEN TRUE)
        else ()
            message(FATAL_ERROR "Lua library loading enabled, but system support for dlopen not present")
        endif ()
    else ()
        message(FATAL_ERROR "Lua library loading enabled, but system support not present")
    endif ()
endif ()

option(LUA_LOCALE_SUPPORT "Lua will use platform locale support" ON)

if (MSVC)
    # Debug postfix
    set(CMAKE_DEBUG_POSTFIX "d")
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Defaulting build type to Release as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()
