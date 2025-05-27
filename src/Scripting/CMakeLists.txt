add_library(Scripting
    LuaUtils.h
    ScriptManager.h
    ScriptManager.cpp
)

target_include_directories(Scripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
