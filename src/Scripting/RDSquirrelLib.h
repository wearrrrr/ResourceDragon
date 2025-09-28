#include "SQUtils.h"
#include "squirrel.h"

namespace RDSquirrelLib {
    static inline SQInteger read_bytes(HSQUIRRELVM vm) {
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

    static inline SQInteger read_u16_le(HSQUIRRELVM vm) {
        SQUserPointer ptr;
        SQInteger offset;

        if (SQ_FAILED(sq_getuserpointer(vm, 2, &ptr)))
            return sq_throwerror(vm, "Expected userpointer as first argument");
        if (SQ_FAILED(sq_getinteger(vm, 3, &offset)))
            return sq_throwerror(vm, "Expected offset as second argument");

        u8* data = (u8*)ptr;

        SQInteger value = data[offset] | (data[offset + 1] << 8);

        sq_pushinteger(vm, value);
        return 1;
    }

    static inline SQInteger read_u32_le(HSQUIRRELVM vm) {
        SQUserPointer ptr;
        SQInteger offset;

        if (SQ_FAILED(sq_getuserpointer(vm, 2, &ptr))) return sq_throwerror(vm, "Expected userpointer as first argument");
        sq_getinteger(vm, 3, &offset);

        u8* data = (u8*)ptr;
        SQInteger value = (data[offset + 3] << 24) | (data[offset + 2] << 16) | (data[offset + 1] << 8) | data[offset];
        sq_pushinteger(vm, value);
        return 1;
    }

    static inline void RegisterAllFuncs(HSQUIRRELVM vm) {
        sq_pushroottable(vm);

        sq_pushstring(vm, SC("read_bytes"), -1);
        sq_newclosure(vm, RDSquirrelLib::read_bytes, 0);
        sq_newslot(vm, -3, SQFalse);

        sq_pushstring(vm, SC("read_u16_le"), -1);
        sq_newclosure(vm, RDSquirrelLib::read_u16_le, 0);
        sq_newslot(vm, -3, SQFalse);

        sq_pushstring(vm, SC("read_u32_le"), -1);
        sq_newclosure(vm, RDSquirrelLib::read_u32_le, 0);
        sq_newslot(vm, -3, SQFalse);

        sq_pop(vm, 1);
    }
}
