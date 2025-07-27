#pragma once

#include "squirrel_all.h"
#include "util/Logger.h"

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
    static inline SQInteger push_buffer_as_array(HSQUIRRELVM vm, const uint8_t* data, size_t size) {
        sq_newarray(vm, 0);
        for (size_t i = 0; i < size; ++i) {
            sq_pushinteger(vm, data[i]);
            sq_arrayappend(vm, -2);
        }
        return 1;
    }
}
