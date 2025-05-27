cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Oz -Wno-unused-parameter -Wno-missing-designated-field-initializers")
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -s")
# set(CMAKE_CXX_FLAGS "-g")
# set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ResourceDragon VERSION 0.1)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(Freetype)
find_package(ZLIB REQUIRED)

add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_mixer EXCLUDE_FROM_ALL)
add_subdirectory(vendored/lua EXCLUDE_FROM_ALL)

if (NOT WIN32)
    set(IMGUI_FREETYPE_CXX vendored/imgui/misc/freetype/imgui_freetype.cpp)
endif()

set(IMGUI_SRC
        vendored/imgui/backends/imgui_impl_sdl3.cpp
        vendored/imgui/backends/imgui_impl_opengl3.cpp
        vendored/imgui/imgui.cpp
        vendored/imgui/imgui_draw.cpp
        vendored/imgui/imgui_tables.cpp
        vendored/imgui/imgui_widgets.cpp
        ${IMGUI_FREETYPE_CXX}

)

include_directories(vendored/imgui)
include_directories(vendored/imgui/backends/)
include_directories(vendored/gles3)
if (NOT WIN32)
    include_directories(/usr/include/freetype2  -I/usr/local/include/freetype2)
endif()

add_subdirectory(src/Scripting)
add_subdirectory(src/GUI)

add_executable(ResourceDragon
    ${IMGUI_SRC}
    src/main.cpp

    src/ArchiveFormats/ElfFile.cpp
    src/ArchiveFormats/ExeFile.cpp

    src/ArchiveFormats/HSP/hsp.cpp
    src/ArchiveFormats/PFS/pfs.cpp
    src/ArchiveFormats/NitroPlus/npa.cpp
    src/ArchiveFormats/NitroPlus/mpk.cpp
    src/ArchiveFormats/SonicAdv/pak.cpp
    src/ArchiveFormats/Touhou/thdat.cpp
    src/ArchiveFormats/XP3/xp3.cpp
)

target_include_directories(ResourceDragon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(OPENGL_LIB_NAME OpenGL)
set(FREETYPE freetype)
if (WIN32)
    set(OPENGL_LIB_NAME opengl32)
    set(FREETYPE "")
endif()

# Internal libs
target_link_libraries(ResourceDragon PRIVATE GUI Scripting)
# Third party libs
target_link_libraries(ResourceDragon PRIVATE ${FREETYPE} ZLIB::ZLIB SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer lua ${OPENGL_LIB_NAME})
