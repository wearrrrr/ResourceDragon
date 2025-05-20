set(LUA_LANGUAGE "C" CACHE STRING "Language in which to compile Lua source code (C or CXX)")
set_property(CACHE LUA_LANGUAGE PROPERTY STRINGS "C" "CXX")
if (LUA_LANGUAGE STREQUAL "C")
    set(LUA_LANGUAGE_SUPPORT "C")
    set(LUA_PROJECT_LANGUAGES "C")
elseif (LUA_LANGUAGE STREQUAL "CXX")
    set(LUA_LANGUAGE_SUPPORT "C++")
    set(LUA_PROJECT_LANGUAGES "C" "CXX")
else ()
    set(LUA_LANGUAGE_SUPPORT "Invalid")
    message(FATAL_ERROR "Invalid language: ${LUA_LANGUAGE}")
endif ()

option(LUA_USE_C89 "Restrict C language usage to C89" OFF)
# This option is only used for C
if (LUA_LANGUAGE STREQUAL CXX)
    set(LUA_USE_C89 OFF)
endif()

option(LUA_USE_APICHECK "Enable additional consistency checks for debugging" OFF)

option(LUA_USE_LONGJMP "Use longjmp in place of C++ exceptions (when compiling as C++)" OFF)

option(LUA_USE_LTESTS "Use ltests for running unit tests" OFF)