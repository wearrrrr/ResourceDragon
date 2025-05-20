# Copyright © 2023 Roger Leigh
#
# Inspired by LuaDist CMake build (https://github.com/LuaDist/lua)
# Copyright © 2007-2015 LuaDist.
# Created by Peter Drahoš, Peter Kapec
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
##############################################################################

# Minimum CMake and policy version
cmake_minimum_required(VERSION 3.16.0)
cmake_policy(VERSION 3.16.0)

# Search for additional CMake modules in cmake/
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(LuaLanguage)

project(lua LANGUAGES ${LUA_PROJECT_LANGUAGES} VERSION 5.4.7)

include(LuaPlatform)
include(LuaLineEditor)
include(GNUInstallDirs)
find_package(CMath)

set(LUAI_MAXCCALLS 200)
if (LUA_USE_LTESTS)
    set(LUAI_MAXCCALLS 180)
endif()
if (WIN32)
    if (LUA_USE_LTESTS)
        set(LUAI_MAXCCALLS 40)
    else ()
        set(LUAI_MAXCCALLS 150)
    endif ()
endif ()

enable_testing()

# add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(tests)

message(STATUS "Lua ${PROJECT_VERSION} is now configured for ${CMAKE_SYSTEM}!")
# message(STATUS "  Platform:                               ${LUA_PLATFORM_SUPPORT} (${LUA_PLATFORM} requested)")
# message(STATUS "  C compiler:                             ${CMAKE_C_COMPILER}")
# message(STATUS "  C++ compiler:                           ${CMAKE_CXX_COMPILER}")
# message(STATUS "  Language for compilation:               ${LUA_LANGUAGE_SUPPORT}")
# message(STATUS "  Use longjmp in place of C++ exceptions: ${LUA_USE_LONGJMP}")
# message(STATUS "  Restrict C to C89:                      ${LUA_USE_C89}")
# message(STATUS "  Additional debug consistency checks:    ${LUA_USE_APICHECK}")
# message(STATUS "  Use ltests:                             ${LUA_USE_LTESTS}")
# message(STATUS "  Build shared libraries:                 ${BUILD_SHARED_LIBS}")
# message(STATUS "  Library loader support:                 ${LUA_LIBRARY_LOADER_SUPPORT} (${LUA_LIBRARY_LOADER} requested)")
# message(STATUS "  Line editor support:                    ${LUA_LINE_EDITING_SUPPORT} (${LUA_LINE_EDITOR} requested)")
# message(STATUS "  Locale support:                         ${LUA_LOCALE_SUPPORT}")
# message(STATUS "  Installation directory:                 ${prefix}")
# message(STATUS "  Test selection:                         ${LUA_TESTS}")
