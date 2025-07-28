#pragma once

#include "squirrel_all.h"
#include <util/int.h>
#include <util/Logger.h>

// idk what this macro stands for, but probably SQChar
#define SC _SC
#define SQ_SUCCESS SQ_SUCCEEDED

namespace SQUtils {
    static inline void CallFunction(HSQUIRRELVM vm, const char *name) {
        sq_pushstring(vm, SC(name), -1);
        if (SQ_SUCCESS(sq_get(vm, -2))) {
            sq_pushroottable(vm);

            if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue))) {
                Logger::error("Failed to call %s()!", name);
                sqstd_printcallstack(vm);
            }
        } else {
            Logger::error("Function '%s' not found.", name);
        }
    }
    inline SQInteger read_bytes(HSQUIRRELVM vm) {
        SQUserPointer ptr;
        SQInteger offset, length;

        if (SQ_FAILED(sq_getuserpointer(vm, 2, &ptr))) return sq_throwerror(vm, "Expected userpointer as first argument");
        sq_getinteger(vm, 3, &offset);
        sq_getinteger(vm, 4, &length);

        u8* data = (u8*)ptr;
        sq_newarray(vm, 0);
        for (SQInteger i = 0; i < length; ++i) {
            sq_pushinteger(vm, data[offset + i]);
            sq_arrayappend(vm, -2);
        }
        return 1;
    }
}
