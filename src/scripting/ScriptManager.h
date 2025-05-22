#pragma once

#include <string>
#include <lua.hpp>
#include "../ArchiveFormats/ArchiveFormat.h"
#include "../util/Logger.h"

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
    uint32_t sig = 0x90909090;

    LuaArchiveFormat(lua_State *state, const char *canHandleFile = "RD__CanHandleFile", const char *tryOpen = "RD__TryOpen", const char *getTag = "RD__GetTag") : m_state(state) {
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
