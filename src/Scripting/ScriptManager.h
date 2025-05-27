#pragma once

#include "lua.hpp"
#include <string>

#include "LuaUtils.h"
#include "../ArchiveFormats/ArchiveFormat.h"
#include "../util/Logger.h"

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

// This template exists to reduce code duplication while also not severly kneecapping runtime performance
template <typename T>
inline T Lua_ConvertTo(lua_State *state) {
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
}

inline bool Lua_GetFunction(lua_State *state, const char *name) {
    int func = lua_getglobal(state, name);
    if (!lua_isfunction(state, -1)) {
        lua_pop(state, -1);
        Logger::error("Unable to find function %s in file, plugin will not load!", name);
        return false;
    }
    return true;
}

class LuaArchiveFormat : public ArchiveFormat {
    lua_State *m_state;
    int lCanHandleRef;
    int lTryOpenRef;
    int lGetTagRef;

public:
    LuaArchiveFormat(lua_State *state, uint32_t signature, const char *canHandleFile = "RD__CanHandleFile", const char *tryOpen = "RD__TryOpen", const char *getTag = "RD__GetTag") : m_state(state) {
        if (!Lua_GetFunction(m_state, canHandleFile)) return;
        lCanHandleRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
        if (!Lua_GetFunction(m_state, tryOpen)) return;
        lTryOpenRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
        if (!Lua_GetFunction(m_state, getTag)) return;
        lGetTagRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
    }

    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, lCanHandleRef);

        lua_pushlstring(m_state, (const char*)buffer, size);
        lua_pushinteger(m_state, size);
        lua_pushstring(m_state, ext.c_str());

        if (lua_pcall(m_state, 3, 1, 0) != LUA_OK) {
            const char *err = lua_tostring(m_state, -1);
            lua_pop(m_state, 1);
            Logger::error(std::string("Lua error: ") + err);
        }

        bool result = lua_toboolean(m_state, -1);
        lua_pop(m_state, 1);
        return result;
    }

    ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override {
        return nullptr;
    };

    std::string GetTag() const override {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, lGetTagRef);

        if (lua_pcall(m_state, 0, 1, 0) != LUA_OK) {
            Logger::error("Failed to call GetTag in lua code! This is a bug.");
            return "GETTAG_FAIL";
        }

        return lua_tostring(m_state, -1);
    };

    ~LuaArchiveFormat() {
        luaL_unref(m_state, LUA_REGISTRYINDEX, lCanHandleRef);
        luaL_unref(m_state, LUA_REGISTRYINDEX, lTryOpenRef);
        luaL_unref(m_state, LUA_REGISTRYINDEX, lGetTagRef);
    }
};

class ScriptManager {
    lua_State *m_state;
    public:
        ScriptManager() {
            m_state = luaL_newstate();
            if (m_state == nullptr) {
                Logger::error("Failed to create Lua state!");
                return;
            }

            luaL_openlibs(m_state);
        }
        ~ScriptManager() {
            Logger::log("Closing lua...");
            lua_close(m_state);
        }

        template <typename T>
        inline T CallLuaMethod(const std::string &name, int argc, int retc, int kfunc = 0);

        void LoadFile(std::string path);
        LuaArchiveFormat *Register();
};
