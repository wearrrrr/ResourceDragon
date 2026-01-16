add_library(Scripting STATIC
    ScriptManager.cpp
    SquirrelArc.cpp
)

target_include_directories(Scripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Scripting PRIVATE SDK util squirrel_static sqstdlib_static)

install(TARGETS Scripting
    LIBRARY DESTINATION lib
)
