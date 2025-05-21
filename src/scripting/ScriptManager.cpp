
#include "ScriptManager.h"



void ScriptManager::executeFile(std::string path) {
    Logger::log("Executing %s...", path.c_str());

    if (luaL_dofile(m_state, path.c_str()) != LUA_OK) {
        Logger::error("Error executing Lua: %s", lua_tostring(m_state, -1));
        lua_pop(m_state, 1);
    }

    lua_close(m_state);

    return;
}
