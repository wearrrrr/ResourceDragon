cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)

if (DEBUG)
    set(CMAKE_CXX_FLAGS "-g")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
else()
    set(CMAKE_CXX_FLAGS
    "-Wall -Wextra -O3 -Wno-unused-parameter -Wno-missing-designated-field-initializers"
)
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ResourceDragon VERSION 0.1)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(Freetype)
find_package(ZLIB REQUIRED)
find_package(libzip)

add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_mixer EXCLUDE_FROM_ALL)
add_subdirectory(vendored/lua EXCLUDE_FROM_ALL)

set(IMGUI_SRC
        vendored/imgui/backends/imgui_impl_sdl3.cpp
        vendored/imgui/backends/imgui_impl_opengl3.cpp
        vendored/imgui/imgui.cpp
        vendored/imgui/imgui_draw.cpp
        vendored/imgui/imgui_tables.cpp
        vendored/imgui/imgui_widgets.cpp
        vendored/imgui/misc/freetype/imgui_freetype.cpp

)

include_directories(vendored/imgui/)
include_directories(vendored/imgui/backends/)
include_directories(vendored/gles3/)
include_directories(vendored/utfcpp/source/)
include_directories(/usr/include/freetype2/)

add_library(util INTERFACE)
target_include_directories(util INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src/ArchiveFormats)
add_subdirectory(src/GUI)
add_subdirectory(src/Scripting)

add_executable(ResourceDragon
    ${IMGUI_SRC}
    src/main.cpp
)

target_include_directories(ResourceDragon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(OPENGL OpenGL)
if (WIN32)
    set(OPENGL opengl32)
endif()

target_link_libraries(ResourceDragon PRIVATE ArchiveFormats GUI Scripting util)
target_link_libraries(ResourceDragon PRIVATE freetype ZLIB::ZLIB SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer zip lua ${OPENGL})

set_target_properties(ResourceDragon PROPERTIES
    BUILD_RPATH "$ORIGIN/lib"
    INSTALL_RPATH "$ORIGIN/lib"
)

install(TARGETS ResourceDragon
    RUNTIME DESTINATION .
)

install(CODE "
    file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/lib TYPE FILE
         FILES ${CMAKE_BINARY_DIR}/vendored/SDL_mixer/libSDL3_mixer.so.0.0.0
         RENAME libSDL3_mixer.so.0)
")
install(CODE "
    file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/lib TYPE FILE
         FILES ${CMAKE_BINARY_DIR}/vendored/SDL_image/libSDL3_image.so.0.3.0
         RENAME libSDL3_image.so.0)
")

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
set(CPACK_PACKAGE_CONTACT "you@example.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ResourceDragon asset viewer")
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
# set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)
