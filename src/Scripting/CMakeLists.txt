add_library(Scripting
    ScriptManager.cpp
)

target_include_directories(Scripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(/usr/include/lua5.4/)

target_link_libraries(Scripting PRIVATE util lua_static)

install(TARGETS Scripting
    LIBRARY DESTINATION lib
)
