cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")

project(ResourceDragon VERSION 0.1)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

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
)

include_directories(vendored/imgui)
include_directories(vendored/imgui/backends/)
include_directories(vendored/gles3)

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

    src/GUI/DirectoryNode.h
    src/GUI/DirectoryNode.cpp
    src/GUI/Utils.h
    src/GUI/Utils.cpp
    src/GUI/Image.h
    src/GUI/Image.cpp
    src/GUI/Theme/Themes.h
    src/GUI/Theme/BessDark.cpp

    src/util/Logger.h

    vendored/stb_image/stb_image.h
    
)

target_link_options(ResourceDragon PRIVATE -s)
target_compile_options(ResourceDragon PRIVATE -O3 -static)

set(OPENGL_LIB_NAME OpenGL)
if (WIN32)
set(OPENGL_LIB_NAME opengl32)
endif()

target_link_libraries(ResourceDragon PRIVATE SDL3::SDL3 ${OPENGL_LIB_NAME})