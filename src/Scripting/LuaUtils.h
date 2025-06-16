#pragma once

#include <string>
#include "lua.hpp"
#include <util/Logger.h>

class LuaUtils {
public:
    static inline bool Lua_GetFunction(lua_State *state, const char *name) {
        lua_getglobal(state, name);
        if (!lua_isfunction(state, -1)) {
            lua_pop(state, -1);
            Logger::error("Unable to find function %s in file, plugin will not load!", name);
            return false;
        }
        return true;
    }

    template <typename T>
    static inline T CallLuaMethod(lua_State *state, const std::string &name, int argc, int retc, int kfunc = 0) {
        lua_getglobal(state, name.c_str());

        if (!lua_isfunction(state, -1)) {
            Logger::error("Unable to find needed function: %s!", name.c_str());
        }

        if (lua_pcall(state, argc, retc, kfunc) != LUA_OK) {
            Logger::error("[Lua] Error calling function `%s`: %s", name.c_str(), lua_tostring(state, -1));
            lua_pop(state, 1);
            return T();
        }

        if (retc > 0) {
            return Lua_ConvertTo<T>(state);
        }

        return T();
    };

    // This template exists to reduce code duplication while also not severly kneecapping runtime performance
    template <typename T>
    static inline T Lua_ConvertTo(lua_State *state) {
        if constexpr (std::is_same_v<T, int>) {
            auto result = lua_tointeger(state, -1);
            lua_pop(state, -1);
            return result;
        } else if constexpr (std::is_same_v<T, double>) {
            auto result = lua_tonumber(state, -1);
            lua_pop(state, -1);
            return result;
        } else if constexpr (std::is_same_v<T, bool>) {
            auto result = lua_toboolean(state, -1);
            lua_pop(state, -1);
            return result;
        } else if constexpr (std::is_same_v<T, std::string>) {
            const char* str = lua_tostring(state, -1);
            lua_pop(state, -1);
            return str ? std::string(str) : std::string("");
        } else {
            static_assert(false, "Unable to deduce proper type!");
        }
    };

    static void dumpstack (lua_State *L) {
      int top = lua_gettop(L);
      Logger::log("[Lua] Stack dump:");
      for (int i = 1; i <= top; i++) {
        printf("[Lua] %d:\t%s\t", i, luaL_typename(L, i));
        switch (lua_type(L, i)) {
          case LUA_TNUMBER:
            printf("%g\n", lua_tonumber(L, i));
            break;
          case LUA_TSTRING:
            printf("%s\n", lua_tostring(L, i));
            break;
          case LUA_TBOOLEAN:
            printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
            break;
          case LUA_TNIL:
            printf("%s\n", "(nil)");
            break;
          default:
            printf("%p\n", lua_topointer(L,i));
            break;
        }
      }
    }
};
