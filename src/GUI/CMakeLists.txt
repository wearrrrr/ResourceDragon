add_library(GUI
    Audio.h
    Audio.cpp
    Clipboard.h
    Clipboard.cpp
    DirectoryNode.h
    DirectoryNode.cpp
    Image.h
    Image.cpp
    TextEditor/TextEditor.h
    TextEditor/TextEditor.cpp
    Theme/BessDark.cpp
    Theme/Themes.h
    Utils.h
    Utils.cpp
)

target_include_directories(GUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(GUI PRIVATE SDL3_mixer::SDL3_mixer)
