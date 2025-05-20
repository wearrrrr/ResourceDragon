set(lua_library_sources
        lapi.c
        lauxlib.c
        lbaselib.c
        lcode.c
        lcorolib.c
        lctype.c
        ldblib.c
        ldebug.c
        ldo.c
        ldump.c
        lfunc.c
        lgc.c
        linit.c
        liolib.c
        llex.c
        lmathlib.c
        lmem.c
        loadlib.c
        lobject.c
        lopcodes.c
        loslib.c
        lparser.c
        lstate.c
        lstring.c
        lstrlib.c
        ltable.c
        ltablib.c
        ltm.c
        lundump.c
        lutf8lib.c
        lvm.c
        lzio.c
)

set(lua_library_private_headers
        lapi.h
        lauxlib.h
        lcode.h
        lctype.h
        ldebug.h
        ldo.h
        lfunc.h
        lgc.h
        ljumptab.h
        llex.h
        llimits.h
        lmem.h
        lobject.h
        lopcodes.h
        lopnames.h
        lparser.h
        lprefix.h
        lstate.h
        lstring.h
        ltable.h
        ltm.h
        lua.h
        lualib.h
        lundump.h
        lvm.h
        lzio.h
)

set(lua_library_public_headers
        lauxlib.h
        lua.h
        lua.hpp
        "${CMAKE_CURRENT_BINARY_DIR}/luaconf.h"
        lualib.h
)

configure_file(luaconf.h.in "${CMAKE_CURRENT_BINARY_DIR}/luaconf.h")

# Some compilers don't like compiling .c files as .cpp, so make a copy of each C file with a .cpp extension
if (LUA_LANGUAGE STREQUAL "CXX")
    set(lua_library_cxx_sources)
    foreach (src IN LISTS lua_library_sources)
        set(dest "${CMAKE_CURRENT_BINARY_DIR}/${src}")
        string(REGEX REPLACE "\\.c" ".cpp" dest "${dest}")
        configure_file("${src}" "${dest}" COPYONLY)
        list(APPEND lua_library_cxx_sources "${dest}")
    endforeach ()
endif ()

add_library(lua_library)

if (LUA_LANGUAGE STREQUAL "C")
    target_sources(lua_library PRIVATE ${lua_library_sources})
elseif (LUA_LANGUAGE STREQUAL "CXX")
    target_sources(lua_library PRIVATE ${lua_library_cxx_sources})
endif ()
target_sources(lua_library PRIVATE ${lua_library_private_headers} ${lua_library_public_headers})

set_target_properties(lua_library PROPERTIES OUTPUT_NAME "lua-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
target_include_directories(lua_library PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}$<SEMICOLON>${CMAKE_CURRENT_BINARY_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lua${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}>"
)
target_link_libraries(lua_library PRIVATE CMath::CMath)
if (LUA_USE_DLOPEN)
    target_link_libraries(lua_library PRIVATE ${CMAKE_DL_LIBS})
endif ()
if (LUA_USE_LTESTS)
    target_compile_definitions(lua_library PUBLIC LUA_USER_H="${PROJECT_SOURCE_DIR}/tests/ltests/ltests.h")
    target_include_directories(lua_library PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests/ltests>")
    target_sources(lua_library PRIVATE "${PROJECT_SOURCE_DIR}/tests/ltests/ltests.c")
endif ()
install(TARGETS lua_library
        EXPORT Lua
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT "runtime"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lua${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}")
install(EXPORT Lua
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake"
        COMPONENT "development")
install(FILES ${lua_library_public_headers}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lua${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}"
        COMPONENT "development")

# luac uses private symbols from the lua library, so create a static build of the library
# as a workaround.
if (BUILD_SHARED_LIBS)
    add_library(lua_library_static STATIC)

    if (LUA_LANGUAGE STREQUAL "C")
        target_sources(lua_library_static PRIVATE ${lua_library_sources})
    elseif (LUA_LANGUAGE STREQUAL "CXX")
        target_sources(lua_library_static PRIVATE ${lua_library_cxx_sources})
    endif ()
    target_sources(lua_library_static PRIVATE ${lua_library_private_headers} ${lua_library_public_headers})

    set_target_properties(lua_library_static PROPERTIES OUTPUT_NAME "lua_static-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    target_include_directories(lua_library_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(lua_library_static PUBLIC CMath::CMath)
    if (LUA_USE_DLOPEN)
        target_link_libraries(lua_library_static PRIVATE ${CMAKE_DL_LIBS})
    endif ()
else ()
    add_library(lua_library_static ALIAS lua_library)
endif ()

add_executable(lua_bin)
if(LUA_LANGUAGE STREQUAL "C")
    target_sources(lua_bin PRIVATE lua.c)
elseif(LUA_LANGUAGE STREQUAL "CXX")
    # Some compilers don't like compiling .c files as .cpp, so make a copy of each C file with a .cpp extension
    configure_file("lua.c" "${CMAKE_CURRENT_BINARY_DIR}/lua.cpp" COPYONLY)
    target_sources(lua_bin PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lua.cpp)
endif()
set_target_properties(lua_bin PROPERTIES OUTPUT_NAME "lua")
target_link_libraries(lua_bin PRIVATE lua_library lua_readline)
install(TARGETS lua_bin RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
        COMPONENT "runtime")


add_executable(luac_bin)
if(LUA_LANGUAGE STREQUAL "C")
    target_sources(luac_bin PRIVATE luac.c)
elseif(LUA_LANGUAGE STREQUAL "CXX")
    # Some compilers don't like compiling .c files as .cpp, so make a copy of each C file with a .cpp extension
    configure_file("luac.c" "${CMAKE_CURRENT_BINARY_DIR}/luac.cpp" COPYONLY)
    target_sources(luac_bin PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/luac.cpp)
endif()
set_target_properties(luac_bin PROPERTIES OUTPUT_NAME "luac")
target_link_libraries(luac_bin PRIVATE lua_library_static)
install(TARGETS luac_bin RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
        COMPONENT "runtime")
