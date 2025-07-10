add_library(GUI
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

target_link_libraries(GUI PRIVATE SDL3_mixer::SDL3_mixer util)
