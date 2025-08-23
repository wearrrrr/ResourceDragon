include(FetchContent)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_STANDARD 23)

project(ResourceDragon VERSION 0.1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    if (DEBUG)
        set(CMAKE_CXX_FLAGS "-g")
        add_compile_definitions(DEBUG)
    else()
        set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -Wno-unused-parameter -Wno-missing-designated-field-initializers")
        set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")
        if (EMSCRIPTEN)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_ZLIB=1 -sUSE_FREETYPE=1 -sDISABLE_EXCEPTION_CATCHING=0")
            # to include tests/ add --preload-file ${CMAKE_SOURCE_DIR}/tests/@/tests
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY --preload-file ${CMAKE_SOURCE_DIR}/tests/@/tests --preload-file ${CMAKE_SOURCE_DIR}/fonts/@/fonts --preload-file ${CMAKE_SOURCE_DIR}/scripts/@/scripts")
        endif()
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld -lbacktrace")
else()
    set(CMAKE_CXX_FLAGS "-mwindows -g")
endif()

if (NOT DEBUG)
    set(LTO ON)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT EMSCRIPTEN)
    message("Downloading freetype...")
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG        aa2ee67b3d95585a0bfc56a834544a502853f584
    )
    FetchContent_MakeAvailable(freetype)
    set(FREETYPE_INCLUDE_DIRS ${freetype_SOURCE_DIR}/include)
    set(FREETYPE_LIBRARIES freetype)

    message("Downloading zlib...")
    include(FetchContent)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG        v1.3.1
    )
    FetchContent_MakeAvailable(zlib)
    set(ZLIB_INCLUDE_DIR ${zlib_BINARY_DIR})
    set(ZLIB_LIBRARY zlib)

    set(ZLIB_INCLUDE_DIR "${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
    set(ZLIB_LIBRARY zlib CACHE STRING "" FORCE)
    set(ZLIB_LIBRARIES zlib CACHE STRING "" FORCE)

    if (DEBUG)
        FetchContent_Declare(
            cpptrace
            GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
            GIT_TAG        v1.0.4
        )
        FetchContent_MakeAvailable(cpptrace)
        set(CPPTRACE_INCLUDE_DIR ${cpptrace_SOURCE_DIR}/include)
        set(CPPTRACE_LIBRARIES cpptrace::cpptrace)
    endif()
endif()

add_subdirectory(vendored/SDL)
add_subdirectory(vendored/SDL_image)
add_subdirectory(vendored/SDL_mixer)

set(DISABLE_DYNAMIC ON)
add_subdirectory(vendored/squirrel)
set(DISABLE_DYNAMIC)

add_subdirectory(vendored/thlib)

# silence squirrel compiler warnings, not my code, not my problem :)
target_compile_options(squirrel_static BEFORE PRIVATE -w)

set(IMGUI_SRC
        vendored/imgui/backends/imgui_impl_sdl3.cpp
        vendored/imgui/backends/imgui_impl_opengl3.cpp
        vendored/imgui/imgui.cpp
        vendored/imgui/imgui_demo.cpp
        vendored/imgui/imgui_draw.cpp
        vendored/imgui/imgui_tables.cpp
        vendored/imgui/imgui_widgets.cpp
        vendored/imgui/misc/freetype/imgui_freetype.cpp
)
include_directories(vendored/)
include_directories(vendored/imgui/)
include_directories(vendored/imgui/backends/)
include_directories(vendored/gles3/)
include_directories(vendored/squirrel/include/)
include_directories(${CPPTRACE_INCLUDE_DIR})

add_library(util INTERFACE)
target_include_directories(util INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src/ArchiveFormats)
add_subdirectory(src/GUI)
add_subdirectory(src/Scripting)

add_executable(ResourceDragon
    ${IMGUI_SRC}
    src/main.cpp
    src/state.cpp
)

target_include_directories(ResourceDragon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(OPENGL GL)
if (WIN32)
    set(OPENGL opengl32)
endif()

set(ZLIB z)
if (WIN32)
    set(ZLIB "")
endif()

if (DEBUG)
    target_link_libraries(ResourceDragon PRIVATE ${CPPTRACE_LIBRARIES})
endif()

target_link_libraries(ResourceDragon PRIVATE ArchiveFormats GUI util Scripting freetype squirrel_static sqstdlib_static ${OPENGL} SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer thlib ${ZLIB})

if (LTO)
    set_target_properties(ResourceDragon PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if (LINUX)
set_target_properties(ResourceDragon PROPERTIES
    INSTALL_RPATH "$ORIGIN/lib:/lib:/usr/lib:/usr/lib64"
)
endif()

if (EMSCRIPTEN)
    set_target_properties(ResourceDragon PROPERTIES SUFFIX ".html")
endif()

install(TARGETS ResourceDragon
    RUNTIME DESTINATION .
)

if (WIN32)
    install(FILES
        ${CMAKE_BINARY_DIR}/vendored/SDL/SDL3.dll
        ${CMAKE_BINARY_DIR}/vendored/SDL_image/SDL3_image.dll
        ${CMAKE_BINARY_DIR}/vendored/SDL_mixer/SDL3_mixer.dll
        DESTINATION .
    )
endif()
