if (WIN32 OR ALPINE)
set(IMGUI_SRC
        ../../vendored/imgui/backends/imgui_impl_sdl3.cpp
        ../../vendored/imgui/backends/imgui_impl_opengl3.cpp
        ../../vendored/imgui/imgui.cpp
        ../../vendored/imgui/imgui_draw.cpp
        ../../vendored/imgui/imgui_tables.cpp
        ../../vendored/imgui/imgui_widgets.cpp
        ../../vendored/imgui/misc/freetype/imgui_freetype.cpp
)
else()
set(IMGUI_SRC "")
endif()


add_library(GUI STATIC
    ${IMGUI_SRC}
    Audio.cpp
    Clipboard.cpp
    DirectoryNode.cpp
    Image.cpp
    Markdown.cpp
    PreviewWindow.cpp
    Render.cpp
    TextEditor/TextEditor.cpp
    Themes.cpp
    UIError.cpp
    Utils.cpp

    ../state.cpp
)

target_include_directories(GUI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

set(OPENGL GL)
if (WIN32)
    set(OPENGL opengl32)
endif()

if (MD4C_LIBRARIES)
    target_include_directories(GUI PRIVATE ${MD4C_INCLUDE_DIR})
    target_link_libraries(GUI PRIVATE ${MD4C_LIBRARIES})
endif()

if (LUNASVG_LIBRARIES)
    target_include_directories(GUI PRIVATE ${LUNASVG_INCLUDE_DIR})
    target_link_libraries(GUI PRIVATE ${LUNASVG_LIBRARIES})
endif()

if (CURL_LIBRARIES)
    target_include_directories(GUI PRIVATE ${CURL_INCLUDE_DIR})
    target_link_libraries(GUI PRIVATE ${CURL_LIBRARIES})
endif()

target_link_libraries(GUI PRIVATE SDK SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer freetype util ${OPENGL})

install(TARGETS GUI
    LIBRARY DESTINATION lib
)
