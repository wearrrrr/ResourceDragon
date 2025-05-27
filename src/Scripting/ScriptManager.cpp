
#include "ScriptManager.h"
#include <lua.h>

template <typename T>
inline T ScriptManager::CallLuaMethod(const std::string &name, int argc, int retc, int kfunc) {
    lua_getglobal(m_state, name.c_str());

    if (!lua_isfunction(m_state, -1)) {
        Logger::error("Unable to find needed function: %s!", name.c_str());
    }

    if (lua_pcall(m_state, argc, retc, kfunc) != LUA_OK) {
        Logger::error("[Lua] Error calling function `%s`: %s", name.c_str(), lua_tostring(m_state, -1));
        lua_pop(m_state, 1);
        return T();
    }

    if (retc > 0) {
        return Lua_ConvertTo<T>(m_state);
    }

    return T();
}

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
    CallLuaMethod<int>("register", 0, 1);
    lua_getglobal(m_state, "signature");
    auto sig = lua_tointeger(m_state, -1);
    lua_pop(m_state, -1);
    return new LuaArchiveFormat(m_state, sig);
}
