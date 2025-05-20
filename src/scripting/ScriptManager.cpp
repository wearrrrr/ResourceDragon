
#include "ScriptManager.h"
#include "../util/Logger.h"
#include <lua.hpp>

void ScriptManager::executeFile(std::string path) {
    lua_State *state = luaL_newstate();

    if (state == nullptr) {
        Logger::error("Failed to create Lua state!");
        return;
    }

    luaL_openlibs(state);

    Logger::log("Executing %s...", path.c_str());

    if (luaL_dofile(state, path.c_str()) != LUA_OK) {
        Logger::error("Error executing Lua: %s", lua_tostring(state, -1));
        lua_pop(state, 1);
    }

    lua_close(state);

    return;
}
