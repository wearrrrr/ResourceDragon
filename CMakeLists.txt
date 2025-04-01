cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")

project(ResourceDragon VERSION 0.1)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(Freetype)

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
        vendored/imgui/imgui_demo.cpp
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

    src/common.h

    src/ExtractorManager.h
    
    src/GameRes/Entry.h

    src/ArchiveFormats/ExeFile.h
    src/ArchiveFormats/ExeFile.cpp
    src/ArchiveFormats/ArchiveFormat.h
    src/ArchiveFormats/ArchiveFormat.cpp

    src/ArchiveFormats/HSP/hsp.h
    src/ArchiveFormats/HSP/hsp.cpp

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
    
)

# target_link_options(ResourceDragon PRIVATE -s)
# target_compile_options(ResourceDragon PRIVATE -Oz)

set(OPENGL_LIB_NAME OpenGL)
set(FREETYPE freetype)
if (WIN32)
set(OPENGL_LIB_NAME opengl32)
set(FREETYPE "")
endif()

target_link_libraries(ResourceDragon PRIVATE ${FREETYPE} SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer ${OPENGL_LIB_NAME})