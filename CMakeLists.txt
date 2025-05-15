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

if (NOT WIN32)
    set(IMGUI_FREETYPE_H vendored/imgui/misc/freetype/imgui_freetype.h)
    set(IMGUI_FREETYPE_CXX vendored/imgui/misc/freetype/imgui_freetype.cpp)
endif()

set(IMGUI_SRC
        vendored/imgui/backends/imgui_impl_sdl3.cpp
        vendored/imgui/backends/imgui_impl_sdl3.h
        vendored/imgui/backends/imgui_impl_opengl3_loader.h
        vendored/imgui/backends/imgui_impl_opengl3.cpp
        vendored/imgui/backends/imgui_impl_opengl3.h
        vendored/imgui/imgui.h
        vendored/imgui/imgui.cpp
        vendored/imgui/imgui_draw.cpp
        vendored/imgui/imgui_tables.cpp
        vendored/imgui/imgui_widgets.cpp
        ${IMGUI_FREETYPE_H}
        ${IMGUI_FREETYPE_CXX}

)

include_directories(vendored/imgui)
include_directories(vendored/imgui/backends/)
include_directories(vendored/gles3)
if (NOT WIN32)
    include_directories(/usr/include/freetype2  -I/usr/local/include/freetype2)
endif()


add_executable(ResourceDragon
    ${IMGUI_SRC}
    src/main.cpp
    src/Icons.h

    src/BinaryReader.h
    src/common.h

    src/ExtractorManager.h

    src/GameRes/Entry.h

    src/ArchiveFormats/ArchiveFormat.h
    src/ArchiveFormats/ElfFile.h
    src/ArchiveFormats/ElfFile.cpp
    src/ArchiveFormats/ExeFile.h
    src/ArchiveFormats/ExeFile.cpp

    src/ArchiveFormats/HSP/hsp.h
    src/ArchiveFormats/HSP/hsp.cpp

    src/ArchiveFormats/PFS/pfs.h
    src/ArchiveFormats/PFS/pfs.cpp

    src/ArchiveFormats/NitroPlus/nitroplus.h
    src/ArchiveFormats/NitroPlus/npa.h
    src/ArchiveFormats/NitroPlus/npa.cpp
    src/ArchiveFormats/NitroPlus/mpk.h
    src/ArchiveFormats/NitroPlus/mpk.cpp

    src/ArchiveFormats/SonicAdv/sonicadv.h
    src/ArchiveFormats/SonicAdv/pak.h
    src/ArchiveFormats/SonicAdv/pak.cpp

    src/ArchiveFormats/Touhou/thdat.h
    src/ArchiveFormats/Touhou/thdat.cpp

    src/ArchiveFormats/XP3/Crypt/Crypt.h
    src/ArchiveFormats/XP3/xp3.h
    src/ArchiveFormats/XP3/xp3.cpp

    src/GUI/Audio.h
    src/GUI/Audio.cpp
    src/GUI/Clipboard.h
    src/GUI/Clipboard.cpp
    src/GUI/DirectoryNode.h
    src/GUI/DirectoryNode.cpp
    src/GUI/Image.h
    src/GUI/Image.cpp
    src/GUI/TextEditor/TextEditor.h
    src/GUI/TextEditor/TextEditor.cpp
    src/GUI/Theme/Themes.h
    src/GUI/Theme/BessDark.cpp
    src/GUI/Utils.h
    src/GUI/Utils.cpp

    src/util/Logger.h
    src/util/Text.h

    src/zero_templates.h

    src/md5.h
    src/sha1.h
)

set(OPENGL_LIB_NAME OpenGL)
set(FREETYPE freetype)
if (WIN32)
    set(OPENGL_LIB_NAME opengl32)
    set(FREETYPE "")
endif()

target_link_libraries(ResourceDragon PRIVATE ${FREETYPE} ZLIB::ZLIB SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer ${OPENGL_LIB_NAME})
