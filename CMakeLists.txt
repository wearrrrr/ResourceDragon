cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)
option(SDL3_VENDORED "Use vendored SDL3" OFF)

project(ResourceDragon VERSION 0.1)

if (SDL3_VENDORED)
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
    find_package(SDL3_ttf REQUIRED)
    find_package(imgui REQUIRED)
    find_package(OpenGL REQUIRED)
endif()

add_executable(ResourceDragon 
    src/main.cpp

    src/ExtractorManager.h
    
    src/GameRes/Entry.h

    src/ArchiveFormats/ExeFile.h
    src/ArchiveFormats/ExeFile.cpp
    src/ArchiveFormats/ArchiveFormat.h
    src/ArchiveFormats/ArchiveFormat.cpp

    src/ArchiveFormats/HSP/hsp.h
    src/ArchiveFormats/HSP/hsp.cpp

    src/GUI/DirectoryNode.cpp

    vendored/imgui/imgui_impl_sdl3.cpp
    vendored/imgui/imgui_impl_sdl3.h
    vendored/imgui/imgui_impl_opengl3_loader.h
    vendored/imgui/imgui_impl_opengl3.cpp
    vendored/imgui/imgui_impl_opengl3.h
    
)

set_target_properties(ResourceDragon PROPERTIES LINK_FLAGS_RELEASE -s)
target_compile_options(ResourceDragon PRIVATE -O3)

target_link_libraries(ResourceDragon PRIVATE SDL3::SDL3 imgui OpenGL SDL3_ttf)