macro(add_lua_test)
    set(options)
    set(oneValueArgs NAME SCRIPT)
    set(multiValueArgs OPTIONS)
    cmake_parse_arguments(LUA_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (BUILD_SHARED_LIBS)
        set(LUA_TEST_MODULE_PATH "$<PATH:REMOVE_FILENAME,$<TARGET_FILE:lua_test_lib1>>")
    else ()
        set(LUA_TEST_MODULE_PATH)
    endif ()

    if (LUA_TEST_SCRIPT STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/all.lua")
        set(test_runner "${CMAKE_CURRENT_SOURCE_DIR}/all.lua")
        set(test_script)
    else ()
        set(test_runner "${CMAKE_CURRENT_SOURCE_DIR}/ctest_runner.lua")
        set(test_script "-e_T='${LUA_TEST_SCRIPT}'")
    endif ()

    add_test(NAME test_${LUA_TEST_NAME} COMMAND $<TARGET_FILE:lua_bin> ${LUA_TEST_OPTIONS} ${test_script} ${test_runner}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND_EXPAND_LISTS)
    set_tests_properties(test_${LUA_TEST_NAME} PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<$<BOOL:${WIN32}>:$<TARGET_FILE_DIR:lua_library>>")
    set_tests_properties(test_${LUA_TEST_NAME} PROPERTIES ENVIRONMENT
            "LUA_PLATFORM=${LUA_PLATFORM_SUPPORT};LUA_LANGUAGE=${LUA_LANGUAGE};LUA_LOCALE=${LUA_LOCALE_SUPPORT};LUA_USE_READLINE=$<BOOL:${LUA_USE_READLINE}>;LUA_LIBRARY_LOADER=$<BOOL:${LUA_LIBRARY_LOADER}>;LUA_PATH=${CMAKE_CURRENT_SOURCE_DIR}/?\;${CMAKE_CURRENT_SOURCE_DIR}/?.lua\;\;;LUA_DL_POSTFIX=$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>;LUA_CPATH=${CMAKE_CURRENT_BINARY_DIR}/?${CMAKE_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}\;${LUA_TEST_MODULE_PATH}?${CMAKE_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}\;\;;LUA_TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR};LUA_TEST_LIBRARY_PATH=${LUA_TEST_MODULE_PATH};LUA_RELATIVE_TEST_LIBRARY_PATH=$<PATH:RELATIVE_PATH,${LUA_TEST_MODULE_PATH},${CMAKE_CURRENT_BINARY_DIR}>")
endmacro()

add_subdirectory(libs)

set(LUA_TESTS "Standard" CACHE STRING "Line editor for interactive editing (Readline, Editline, None or Autodetect")
set_property(CACHE LUA_TESTS PROPERTY STRINGS "Basic" "Standard" "Extended")


set(lua_tests_basic
        api
        attrib
        basic
        bitwise
        bwcoercion
        calls
        closure
        code
        constructs
        coroutine
        cstack
        db
        errors
        events
        files
        gc
        gengc
        goto
        literals
        locals
        main
        math
        nextvar
        pm
        sort
        strings
        tpack
        tracegc
        utf8
        vararg
        verybig
        )

set(lua_tests_standard
        big
        )

set(lua_tests_extended
        heavy
        )

set(lua_tests)
if (LUA_TESTS STREQUAL "Basic" OR LUA_TESTS STREQUAL "Standard" OR LUA_TESTS STREQUAL "Extended")
    list(APPEND lua_tests ${lua_tests_basic})
endif ()
if (LUA_TESTS STREQUAL "Standard" OR LUA_TESTS STREQUAL "Extended")
    list(APPEND lua_tests ${lua_tests_standard})
endif ()
if (LUA_TESTS STREQUAL "Extended")
    list(APPEND lua_tests ${lua_tests_extended})
endif ()

foreach (test IN LISTS lua_tests)
    add_lua_test(NAME ${test} SCRIPT ${test}.lua)
endforeach ()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libs/P1")
