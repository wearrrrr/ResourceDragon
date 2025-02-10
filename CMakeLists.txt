cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)

project(ResourceDragon VERSION 0.1)

add_executable(ResourceDragon 
    src/main.cpp

    src/GameRes/Entry.h

    src/ArchiveFormats/ExeFile.h
    src/ArchiveFormats/ExeFile.cpp
    src/ArchiveFormats/ArchiveFormat.h
    src/ArchiveFormats/ArchiveFormat.cpp

    src/ArchiveFormats/HSP/hsp.h
    src/ArchiveFormats/HSP/hsp.cpp
    
)

set_target_properties(ResourceDragon PROPERTIES LINK_FLAGS_RELEASE -s)
target_compile_options(ResourceDragon PRIVATE -O3)