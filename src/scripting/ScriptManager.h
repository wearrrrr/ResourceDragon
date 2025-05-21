#include <string>
#include <lua.hpp>
#include "../util/Logger.h"

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
        void executeFile(std::string path);
};
