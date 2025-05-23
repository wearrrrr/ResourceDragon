
#include "ScriptManager.h"

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
        if constexpr (std::is_same_v<T, int>) {
            auto result = lua_tonumber(m_state, -1);
            lua_pop(m_state, -1);
            return result;
        } else if constexpr (std::is_same_v<T, double>) {
            auto result = lua_tointeger(m_state, -1);
            lua_pop(m_state, -1);
            return result;
        } else if constexpr (std::is_same_v<T, bool>) {
            auto result = lua_toboolean(m_state, -1);
            lua_pop(m_state, -1);
            return result;
        } else if constexpr (std::is_same_v<T, const char*>) {
            const char* str = lua_tostring(m_state, -1);
            lua_pop(m_state, -1);
            return str;
        } else if constexpr (std::is_same_v<T, std::string>) {
            const char* str = lua_tostring(m_state, -1);
            lua_pop(m_state, -1);
            return str ? std::string(str) : std::string("");
        } else {
            static_assert(false, "CallLuaMethod currently does not support this type!");
        }
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
    CallLuaMethod<int>("register", 0, 1); // you expect Lua to define RD__CanHandleFile
    return new LuaArchiveFormat(m_state);
}
