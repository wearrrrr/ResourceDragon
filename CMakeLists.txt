include(FetchContent)

cmake_minimum_required(VERSION 3.10)

set(LTO ON)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_STANDARD 23)

if (NOT WIN32)
    if (DEBUG)
        set(CMAKE_CXX_FLAGS "-g")
        set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
    else()
        set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -Wno-unused-parameter -Wno-missing-designated-field-initializers")
        set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
        if (EMSCRIPTEN)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_ZLIB=1 -sUSE_FREETYPE=1 -sDISABLE_EXCEPTION_CATCHING=0")
            # to include tests/ add --preload-file ${CMAKE_SOURCE_DIR}/tests/@/tests
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY ${CMAKE_SOURCE_DIR}/fonts/@/fonts --preload-file ${CMAKE_SOURCE_DIR}/scripts/@/scripts")
        endif()
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ResourceDragon VERSION 0.1)

if (NOT EMSCRIPTEN)
    find_package(ZLIB QUIET)
    find_package(Freetype QUIET)

    if(NOT Freetype_FOUND OR WIN32)
        message(STATUS "Freetype not found, fetching...")
        FetchContent_Declare(
            freetype
            GIT_REPOSITORY https://github.com/freetype/freetype.git
            GIT_TAG        aa2ee67b3d95585a0bfc56a834544a502853f584
        )
        FetchContent_MakeAvailable(freetype)
        set(FREETYPE_INCLUDE_DIRS ${freetype_SOURCE_DIR}/include)
        set(FREETYPE_LIBRARIES freetype)
    else()
        find_package(PkgConfig REQUIRED)
        message(STATUS "Using system Freetype")
        pkg_check_modules(freetype2 REQUIRED freetype2)
        include_directories(${freetype2_INCLUDE_DIRS})
    endif()

    if(NOT ZLIB_FOUND OR WIN32)
        message(STATUS "Zlib not found, fetching...")

        include(FetchContent)
        FetchContent_Declare(
            zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG        v1.3.1
        )
        FetchContent_MakeAvailable(zlib)
        set(ZLIB_INCLUDE_DIR ${zlib_BINARY_DIR})
        set(ZLIB_LIBRARY zlib)
    else()
        message(STATUS "Using system Zlib")
    endif()

    set(ZLIB_INCLUDE_DIR "${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
    set(ZLIB_LIBRARY zlib CACHE STRING "" FORCE)
    set(ZLIB_LIBRARIES zlib CACHE STRING "" FORCE)
endif()

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_mixer EXCLUDE_FROM_ALL)

set(DISABLE_DYNAMIC ON)
add_subdirectory(vendored/squirrel EXCLUDE_FROM_ALL)
set(DISABLE_DYNAMIC)

add_subdirectory(vendored/thlib EXCLUDE_FROM_ALL)

# silence squirrel compiler warnings, not my code, not my problem :)
target_compile_options(squirrel_static BEFORE PRIVATE -w)

set(IMGUI_SRC
        vendored/imgui/backends/imgui_impl_sdl3.cpp
        vendored/imgui/backends/imgui_impl_opengl3.cpp
        vendored/imgui/imgui.cpp
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

add_library(util INTERFACE)
target_include_directories(util INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src/ArchiveFormats)
add_subdirectory(src/GUI)
add_subdirectory(src/Scripting)

add_executable(ResourceDragon
    ${IMGUI_SRC}
    src/main.cpp
    vendored/sha1.c
)

target_include_directories(ResourceDragon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(OPENGL GL)
if (WIN32)
    set(OPENGL opengl32)
endif()

target_link_libraries(ResourceDragon PRIVATE ArchiveFormats GUI util Scripting)
target_link_libraries(ResourceDragon PRIVATE freetype squirrel_static sqstdlib_static ${OPENGL} SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer thlib z)

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

if (LINUX)
install(CODE "
    file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/lib TYPE FILE
         FILES ${CMAKE_BINARY_DIR}/vendored/SDL_mixer/libSDL3_mixer.so.0.1.0
         RENAME libSDL3_mixer.so.0)
")
install(CODE "
    file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/lib TYPE FILE
         FILES ${CMAKE_BINARY_DIR}/vendored/SDL_image/libSDL3_image.so.0.3.0
         RENAME libSDL3_image.so.0)
")
install(CODE "
    file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/lib TYPE FILE
         FILES ${CMAKE_BINARY_DIR}/vendored/SDL/libSDL3.so.0.3.0
         RENAME libSDL3.so.0)
")
install(CODE "
    file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/lib TYPE FILE
         FILES ${CMAKE_BINARY_DIR}/vendored/libzip/lib/libzip.so.5.5
         RENAME libzip.so.5)
")
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/fonts/
        DESTINATION fonts
        FILES_MATCHING PATTERN "*"
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/
        DESTINATION scripts
        FILES_MATCHING PATTERN "*"
)

set(CPACK_PACKAGE_NAME "ResourceDragon")
set(CPACK_PACKAGE_VERSION "0.1")
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_CONTACT "contact@wearr.dev")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ResourceDragon asset viewer")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)
