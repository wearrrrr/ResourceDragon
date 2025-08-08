set(IMGUI_SRC
        ../../vendored/imgui/backends/imgui_impl_sdl3.cpp
        ../../vendored/imgui/backends/imgui_impl_opengl3.cpp
        ../../vendored/imgui/imgui.cpp
        ../../vendored/imgui/imgui_draw.cpp
        ../../vendored/imgui/imgui_tables.cpp
        ../../vendored/imgui/imgui_widgets.cpp
        ../../vendored/imgui/misc/freetype/imgui_freetype.cpp
)


add_library(GUI
    ${IMGUI_SRC}
    Audio.cpp
    Clipboard.cpp
    DirectoryNode.cpp
    Image.cpp
    ImVec2Util.h
    PreviewWindow.cpp
    TextEditor/TextEditor.cpp
    Theme/BessDark.cpp
    UIError.cpp
    Utils.cpp
)

target_include_directories(GUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(GUI PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer freetype util)

install(TARGETS GUI
    LIBRARY DESTINATION lib
)
