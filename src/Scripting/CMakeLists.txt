add_library(Scripting
    ScriptManager.cpp
)

target_include_directories(Scripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Scripting PRIVATE util lua)

install(TARGETS Scripting
    LIBRARY DESTINATION lib
)
