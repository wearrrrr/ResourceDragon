
#include "ScriptManager.h"
#include "LuaUtils.h"
#include <lua.h>

void ScriptManager::LoadFile(std::string path) {
    Logger::log("[Lua] Loading %s...", path.c_str());

    if (luaL_dofile(m_state, path.c_str()) != LUA_OK) {
        Logger::error("Error executing Lua: %s", lua_tostring(m_state, -1));
        lua_pop(m_state, 1);
        return;
    }

    return;
}

LuaArchiveFormat *ScriptManager::Register() {
    LuaUtils::CallLuaMethod<int>(m_state, "register", 0, 1);
    lua_getglobal(m_state, "signature");
    auto sig = lua_tointeger(m_state, -1);
    lua_pop(m_state, 1);
    return new LuaArchiveFormat(m_state, sig);
}
