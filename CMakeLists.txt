include(FetchContent)
cmake_minimum_required(VERSION 3.10)
project(ResourceDragon VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
   CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(USING_CLANG_CL ON)
    message(STATUS "Using clang-cl")
endif()

if(MINGW)
    add_compile_options(-mwindows)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-missing-designated-field-initializers
    )
elseif(MSVC OR USING_CLANG_CL)
    add_compile_options(/W3 /permissive- -fexceptions)
endif()

if(DEBUG)
    add_compile_definitions(DEBUG)
    if(MSVC OR USING_CLANG_CL)
        add_compile_options(/Zi)
    else()
        add_compile_options(-g)
        set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")
    endif()
else()
    if(MSVC OR USING_CLANG_CL)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
    endif()
endif()

if(ALPINE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

if(EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -sUSE_PTHREADS=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -sUSE_PTHREADS=1 -sUSE_ZLIB=1 -sUSE_FREETYPE=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/fonts/@/fonts --preload-file ${CMAKE_SOURCE_DIR}/scripts/@/scripts -pthread -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=5 -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY")
    if(DEBUG)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sASSERTIONS")
    endif()
endif()

if(MSVC OR USING_CLANG_CL)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

if(NOT DEBUG)
    set(LTO ON)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static" FORCE)
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
    add_compile_definitions(CPPTRACE_ENABLED)
    endif()
endif()


FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.2.0
)
FetchContent_MakeAvailable(fmt)
set(FMT_INCLUDE_DIR ${fmt_SOURCE_DIR}/include/)
set(FMT_LIBRARIES fmt::fmt)

add_subdirectory(vendored/SDL)
set(SDLIMAGE_AVIF OFF CACHE BOOL "AVIF codec for decoding" FORCE)
set(AVIF_CODEC_AOM OFF CACHE BOOL "Use aom codec for decoding" FORCE)
set(AVIF_CODEC_AOM_ENCODE OFF CACHE BOOL "Enable aom encoder" FORCE)
set(AVIF_CODEC_AOM_DECODE OFF CACHE BOOL "Enable aom decoder" FORCE)
set(AVIF_CODEC_DAV1D OFF CACHE BOOL "Use dav1d codec for decoding" FORCE)
add_subdirectory(vendored/SDL_image)
add_subdirectory(vendored/SDL_mixer)

set(DISABLE_DYNAMIC ON)
add_subdirectory(vendored/squirrel)
set(DISABLE_DYNAMIC)


add_subdirectory(vendored/thlib)
set_target_properties(thlib PROPERTIES
    BUILD_SHARED_LIBS OFF
)
add_subdirectory(vendored/lunasvg)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared" FORCE)
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
include_directories(vendored/lunasvg/include/)
include_directories(${FMT_INCLUDE_DIR})
include_directories(${CPPTRACE_INCLUDE_DIR})

add_library(util INTERFACE)
target_include_directories(util INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src/ArchiveFormats)
add_subdirectory(src/GUI)
add_subdirectory(src/Scripting)

set(PLUGIN_CPP "")
if (NOT EMSCRIPTEN)
# add src/Plugins/plugins.cpp to ResourceDragon
set(PLUGIN_CPP "${CMAKE_CURRENT_SOURCE_DIR}/src/Plugins/plugins.cpp")
endif()

add_executable(ResourceDragon
    ${IMGUI_SRC}
    src/main.cpp
    src/state.cpp
    ${PLUGIN_CPP}

    SDK/sdk.cpp
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

if (CPPTRACE_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${CPPTRACE_LIBRARIES})
endif()
if (FMT_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${FMT_LIBRARIES})
endif()
if (LUNASVG_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${LUNASVG_LIBRARIES})
endif()

target_link_libraries(ResourceDragon PRIVATE ArchiveFormats GUI util Scripting freetype lunasvg squirrel_static sqstdlib_static ${OPENGL} SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer thlib ${ZLIB})

if (LTO)
    set_target_properties(ResourceDragon PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
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
