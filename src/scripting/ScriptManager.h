#pragma once

#include <string>
#include <lua.hpp>
#include "../ArchiveFormats/ArchiveFormat.h"
#include "../util/Logger.h"

class LuaArchiveFormat : public ArchiveFormat {
    lua_State *m_state;
    int luaCanHandleRef;

public:
    uint32_t sig = 0x90909090;

    LuaArchiveFormat(lua_State *state, const char* globalFunctionName) : m_state(state) {
        // Push global function to stack
        lua_getglobal(m_state, globalFunctionName);
        if (!lua_isfunction(m_state, -1)) {
            lua_pop(m_state, 1);
            Logger::error("Expected a Lua function for CanHandleFile");
            return;
        }

        // Store a reference to the function in the Lua registry
        luaCanHandleRef = luaL_ref(m_state, LUA_REGISTRYINDEX);
    }

    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, luaCanHandleRef);

        lua_pushlstring(m_state, (const char*)buffer, size);
        lua_pushinteger(m_state, size);
        lua_pushstring(m_state, ext.c_str());

        if (lua_pcall(m_state, 3, 1, 0) != LUA_OK) {
            const char *err = lua_tostring(m_state, -1);
            lua_pop(m_state, 1);
            Logger::error(std::string("Lua error: ") + err);
        }

        bool result = lua_toboolean(m_state, -1);
        lua_pop(m_state, 1); // pop result
        return result;
    }

    ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override {

        return nullptr;
    };

    ~LuaArchiveFormat() {
        luaL_unref(m_state, LUA_REGISTRYINDEX, luaCanHandleRef);
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
