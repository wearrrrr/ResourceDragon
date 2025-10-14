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
        -Wno-deprecated-declarations
    )
elseif(MSVC OR USING_CLANG_CL)
    add_compile_options(-Wno-deprecated-declarations -Wno-unsafe-buffer-usage -Wno-reserved-identifier)
endif()

if(DEBUG)
    add_compile_definitions(DEBUG)
    if(MSVC OR USING_CLANG_CL)
        add_compile_options(/Zi)
    else()
        add_compile_options(-g)
        add_link_options(-rdynamic)
    endif()
else()
    if(MSVC OR USING_CLANG_CL)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
    endif()
endif()

if(ALPINE)
    add_link_options(-static-libgcc -static-libstdc++)
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
        GIT_TAG        4334f009e7d20789cc7ee1224290ea1e22a17b5b
    )
    FetchContent_MakeAvailable(freetype)
    include_directories(${freetype_SOURCE_DIR}/include)

    set(FREETYPE_LIBRARIES freetype)

    if (DEBUG)
    message("Downloading cpptrace...")
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

if (NOT EMSCRIPTEN)
find_package(zlib QUIET)

if (NOT ZLIB_FOUND)
    message("Downloading zlib...")
    set(ZLIB_BUILD_STATIC "ON" CACHE BOOL "Use static zlib library" FORCE)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG        v1.3.1
    )
    FetchContent_MakeAvailable(zlib)
    set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR})
    set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}" CACHE PATH "" FORCE)
endif()

find_package(CURL QUIET)
if (NOT CURL_FOUND)
    message("Downloading curl...")
    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG        curl-8_16_0
    )

    set(BUILD_TESTING OFF CACHE BOOL "Disable curl tests" FORCE)
    set(CURL_DISABLE_TESTS ON CACHE BOOL "Disable curl-specific tests" FORCE)

    set(CURL_DISABLE_FTP ON CACHE BOOL "Disable FTP support" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "Disable LDAP support" FORCE)
    set(CURL_DISABLE_DICT ON CACHE BOOL "Disable DICT support" FORCE)
    set(CURL_DISABLE_TELNET ON CACHE BOOL "Disable TELNET support" FORCE)
    set(CURL_DISABLE_RTSP ON CACHE BOOL "Disable RTSP support" FORCE)
    set(CURL_DISABLE_SMTP ON CACHE BOOL "Disable SMTP support" FORCE)
    set(CURL_DISABLE_IMAP ON CACHE BOOL "Disable IMAP support" FORCE)
    set(CURL_DISABLE_POP3 ON CACHE BOOL "Disable POP3 support" FORCE)
    set(CURL_DISABLE_MQTT ON CACHE BOOL "Disable MQTT support" FORCE)
    set(CURL_DISABLE_SMB ON CACHE BOOL "Disable SMB support" FORCE)
    set(CURL_DISABLE_LDAPS ON CACHE BOOL "Disable LDAPS support" FORCE)

    if(WIN32)
        # https://github.com/curl/curl/issues/16486 .-.
        set(CURL_DISABLE_LIBPSL ON CACHE BOOL "Disable libpsl usage" FORCE)

        set(CURL_USE_SCHANNEL ON CACHE BOOL "Use Windows native SSL backend" FORCE)
    endif()

    FetchContent_MakeAvailable(curl)
    set(CURL_INCLUDE_DIR ${curl_SOURCE_DIR}/include)
endif()

set(CURL_LIBRARIES CURL::libcurl)

endif()

message("Downloading fmt...")
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.2.0
)
FetchContent_MakeAvailable(fmt)
set(FMT_INCLUDE_DIR ${fmt_SOURCE_DIR}/include/)
set(FMT_LIBRARIES fmt::fmt)

message("Downloading md4c...")
FetchContent_Declare(
    md4c
    GIT_REPOSITORY https://github.com/mity/md4c.git
    GIT_TAG        481fbfbdf72daab2912380d62bb5f2187d438408
)
FetchContent_MakeAvailable(md4c)
set(MD4C_INCLUDE_DIR ${md4c_SOURCE_DIR}/src/)
set(MD4C_LIBRARIES md4c)

set(SDL_CAMERA OFF)
set(SDL_JOYSTICK OFF)
set(SDL_HAPTIC OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_TRAY OFF)
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
set(LUNASVG_LIBRARIES lunasvg)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared" FORCE)

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
include_directories(${zlib_BINARY_DIR})
include_directories(${zlib_SOURCE_DIR})
include_directories(${CURL_INCLUDE_DIR})
include_directories(${MD4C_INCLUDE_DIR})

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
    vendored/imgui_md/imgui_md.cpp
    src/main.cpp
    src/state.cpp
    src/Net.cpp
    ${PLUGIN_CPP}
    SDK/sdk.cpp
)

target_compile_options(ResourceDragon PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
    $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/EHsc>
    $<$<CXX_COMPILER_ID:Clang>:-fexceptions>
    $<$<CXX_COMPILER_ID:GNU>:-fexceptions>
)

target_include_directories(ResourceDragon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(OPENGL GL)
if (WIN32)
    set(OPENGL opengl32)
endif()

if (FREETYPE_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${FREETYPE_LIBRARIES})
endif()
if (CPPTRACE_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${CPPTRACE_LIBRARIES})
endif()
if (FMT_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${FMT_LIBRARIES})
endif()
if (MD4C_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${MD4C_LIBRARIES})
endif()
if (LUNASVG_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${LUNASVG_LIBRARIES})
endif()

if (CURL_LIBRARIES)
    target_link_libraries(ResourceDragon PRIVATE ${CURL_LIBRARIES})
endif()

target_link_libraries(ResourceDragon PRIVATE ArchiveFormats GUI util Scripting squirrel_static sqstdlib_static ${OPENGL} SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer thlib)

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
