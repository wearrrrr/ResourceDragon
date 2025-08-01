#pragma once

#include "lua.hpp"
#include <lua.h>
#include <string>

#include "LuaUtils.h"
#include <ArchiveFormats/ArchiveFormat.h>
#include <util/Logger.h>

class LuaArchiveFormat : public ArchiveFormat {
    lua_State *m_state;
    int lCanHandleRef;
    int lTryOpenRef;
    int lGetTagRef;
    int lGetDescriptionRef;

public:
    LuaArchiveFormat(lua_State *state, u32 signature, const char *canHandleFile = "RD__CanHandleFile", const char *tryOpen = "RD__TryOpen", const char *getTag = "RD__GetTag",  const char *getDescription = "RD__GetDescription") : m_state(state) {
        if (!LuaUtils::Lua_GetFunction(m_state, canHandleFile)) return;
        lCanHandleRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
        if (!LuaUtils::Lua_GetFunction(m_state, tryOpen)) return;
        lTryOpenRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
        if (!LuaUtils::Lua_GetFunction(m_state, getTag)) return;
        lGetTagRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
        if (!LuaUtils::Lua_GetFunction(m_state, getDescription)) return;
        lGetDescriptionRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
    }

    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, lCanHandleRef);

        lua_gc(m_state, LUA_GCSTOP, 0);

        lua_pushlstring(m_state, (const char*)buffer, size);
        lua_pushinteger(m_state, size);
        lua_pushstring(m_state, ext.c_str());

        lua_gc(m_state, LUA_GCRESTART, 0);

        if (lua_pcall(m_state, 3, 1, 0) != LUA_OK) {
            const char *err = lua_tostring(m_state, -1);
            lua_pop(m_state, 1);
            lua_gc(m_state, LUA_GCCOLLECT, 0);
            Logger::error(std::string("Lua error: ") + err);
            return false;
        }

        bool result = lua_toboolean(m_state, -1);
        lua_pop(m_state, 1);
        lua_gc(m_state, LUA_GCCOLLECT, 0);
        return result;
    }

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, lTryOpenRef);

        lua_pushlstring(m_state, (const char*)buffer, size);
        lua_pushinteger(m_state, size);
        lua_pushstring(m_state, file_name.c_str());

        if (lua_pcall(m_state, 3, 1, 0) != LUA_OK) {
            const char *err = lua_tostring(m_state, -1);
            lua_pop(m_state, 1);
            lua_gc(m_state, LUA_GCCOLLECT, 0);
            Logger::error(std::string("Lua error: ") + err);
            return nullptr;
        }
        // TODO: Later on, this will return a table when successful, which we can use to construct a proper C++ class.
        lua_toboolean(m_state, -1);
        lua_pop(m_state, 1);

        lua_gc(m_state, LUA_GCCOLLECT, 0);
        return nullptr;
    };

    std::string_view GetTag() const override {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, lGetTagRef);

        if (lua_pcall(m_state, 0, 1, 0) != LUA_OK) {
            Logger::error("Failed to call GetTag in lua code! This is a bug.");
            return "GETTAG_FAIL";
        }

        return lua_tostring(m_state, -1);
    };

    std::string_view GetDescription() const override {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, lGetDescriptionRef);

        if (lua_pcall(m_state, 0, 1, 0) != LUA_OK) {
            Logger::error("Failed to call GetDescription in lua code! This is a bug.");
            return "GETDESC_FAIL";
        }

        return lua_tostring(m_state, -1);
    }

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
