add_library(Scripting
    ScriptManager.cpp
    SquirrelArc.cpp
)

target_include_directories(Scripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Scripting PRIVATE util squirrel sqstdlib)

install(TARGETS Scripting
    LIBRARY DESTINATION lib
)
