#pragma once

#include <limits>
#include <vector>
#include <memory.h>
#include <utility>

#include "squirrel.h"
#include "squirrel_all.h"
#include <util/int.h>
#include <util/Logger.h>


// idk what this macro stands for, but probably SQChar
#define SC _SC
#define SQ_SUCCESS SQ_SUCCEEDED

struct SQUtils {
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

    static inline void push_all(HSQUIRRELVM) {
        // No arguments.
    }

    template <typename T>
    static void push_value(HSQUIRRELVM vm, T&& val) {
        using U = std::remove_cvref_t<T>;

        if constexpr (std::numeric_limits<U>::is_integer && !std::is_same_v<U, bool>) {
            sq_pushinteger(vm, static_cast<SQInteger>(val));
        } else if constexpr (std::is_same_v<U, bool>) {
            sq_pushbool(vm, val);
        } else if constexpr (std::is_floating_point_v<U>) {
            sq_pushfloat(vm, static_cast<SQFloat>(val));
        } else if constexpr (std::is_same_v<U, std::string>) {
            sq_pushstring(vm, val.c_str(), -1);
        } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
            sq_pushstring(vm, val, -1);
        } else if constexpr (std::is_same_v<U, unsigned char*>) {
            sq_pushuserpointer(vm, val);
        } else if constexpr (std::is_same_v<U, std::nullptr_t>) {
            sq_pushnull(vm);
        } else if constexpr (std::is_same_v<U, HSQOBJECT>) {
                sq_pushobject(vm, val);
        } else if constexpr (std::is_pointer_v<U>) {
            static_assert(sizeof(U) == 0, "push_value: unsupported pointer type");
        } else {
            static_assert(sizeof(U) == 0, "push_value: unsupported type passed to Squirrel");
        }
    }

    template <typename T, typename... Rest>
    static void push_all(HSQUIRRELVM vm, T&& arg, Rest&&... rest) {
        push_value(vm, std::forward<T>(arg));
        push_all(vm, std::forward<Rest>(rest)...);
    }

    template <typename... Args>
    static bool call_squirrel_function_in_table(HSQUIRRELVM vm, HSQOBJECT table, const char* func_name, Args&&... args) {
        constexpr int arg_count = sizeof...(Args);

        // [table]
        sq_pushobject(vm, table);

        // [table][func_name]
        sq_pushstring(vm, func_name, -1);

        if (SQ_FAILED(sq_get(vm, -2))) {
            sq_pop(vm, 1); // pop table
            Logger::error("[Squirrel] Squirrel function not found: '%s'", func_name);
            return false;
        }
        // push this
        sq_pushobject(vm, table);

        // push function args
        push_all(vm, std::forward<Args>(args)...); // [table][function][this](...)
        if (SQ_FAILED(sq_call(vm, 1 + arg_count, SQTrue, SQTrue))) {
            sq_pop(vm, 1);
            return false;
        }

        sq_remove(vm, -2);
        return true;
    }

    static const SQChar* GetStringFromStack(HSQUIRRELVM vm, const char *strToRetrieve) {
        sq_pushstring(vm, strToRetrieve, -1);
        if (SQ_SUCCEEDED(sq_get(vm, -2))) {
          const SQChar *str;
          if (SQ_SUCCEEDED(sq_getstring(vm, -1, &str))) {
              sq_pop(vm, 1);
              return str;
          }
        }
        return "";
    }

    static SQInteger GetIntFromStack(HSQUIRRELVM vm, const char *strToRetrieve) {
        sq_pushstring(vm, strToRetrieve, -1);
        if (SQ_SUCCEEDED(sq_get(vm, -2))) {
          SQInteger num;
          if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &num))) {
              sq_pop(vm, 1);
              return num;
          }
        }
        return 0;
    }

    static void print_stack_top_value(std::vector<SQObjectValue>& recursion_vec, HSQUIRRELVM v, int depth) {
        union {
            SQInteger val_int;
            SQFloat val_float;
            SQBool val_bool;
            HSQOBJECT val_obj;
            struct {
                SQUserPointer val_user_ptr;
                SQUserPointer val_type_tag;
            };
            struct {
                const SQChar* val_string;
                SQUnsignedInteger val_closure_params;
                SQUnsignedInteger val_closure_free_vars;
            };
            HSQUIRRELVM val_thread;
        };

        switch (u32 val_type = _RAW_TYPE(sq_gettype(v, -1))) {
            case _RT_NULL:
                printf("null");
                break;
            case _RT_INTEGER:
                sq_getinteger(v, -1, &val_int);
                printf("%" _PRINT_INT_PREC "d", val_int);
                break;
            case _RT_FLOAT:
                sq_getfloat(v, -1, &val_float);
                printf("%f", val_float);
                break;
            case _RT_BOOL:
                sq_getbool(v, -1, &val_bool);
                printf(val_bool ? "true" : "false");
                break;
            case _RT_STRING:
                sq_getstring(v, -1, &val_string);
                printf("\"%s\"", val_string);
                break;
            case _RT_USERDATA:
                sq_getuserdata(v, -1, &val_user_ptr, &val_type_tag);
                printf("UserData %p (%p type, %" _PRINT_INT_PREC "d bytes)", val_user_ptr, val_type_tag, sq_getsize(v, -1));
                break;
            case _RT_CLOSURE: case _RT_NATIVECLOSURE:
                sq_getclosureinfo(v, -1, (SQInteger*)&val_closure_params, (SQInteger*)&val_closure_free_vars);
                sq_getclosurename(v, -1);
                if (_RAW_TYPE(sq_gettype(v, -1)) == _RT_STRING) {
                    sq_getstring(v, -1, &val_string);
                    printf(
                        val_type == _RT_CLOSURE
                            ? "Closure \"%s\" (%" _PRINT_INT_PREC "u params, %" _PRINT_INT_PREC "u free)"
                            : "NativeClosure \"%s\" (%" _PRINT_INT_PREC "u params, %" _PRINT_INT_PREC "u free)",
                            val_string, val_closure_params, val_closure_free_vars
                    );
                } else {
                    printf(
                        val_type == _RT_CLOSURE
                            ? "Closure (%" _PRINT_INT_PREC "u params, %" _PRINT_INT_PREC "u free)"
                            : "NativeClosure (%" _PRINT_INT_PREC "u params, %" _PRINT_INT_PREC "u free)",
                            val_closure_params, val_closure_free_vars
                    );
                }
                sq_pop(v, 1);
                break;
            case _RT_GENERATOR:
                printf("Generator");
                break;
            case _RT_USERPOINTER:
                sq_getuserpointer(v, -1, &val_user_ptr);
                printf("UserPointer (%p)", val_user_ptr);
                break;
            case _RT_THREAD:
                sq_getthread(v, -1, &val_thread);
                printf("Thread (%p)", val_thread);
                break;
            case _RT_FUNCPROTO:
                printf("FuncProto");
                break;
            case _RT_INSTANCE:
                sq_getclass(v, -1);
                sq_remove(v, -2);
            case _RT_CLASS:
                sq_gettypetag(v, -1, &val_type_tag);
                printf(val_type == _RT_CLASS ? "Class (%p type) " : "Instance (%p type) ", val_type_tag);
                goto skip_size_check;

            case _RT_TABLE: case _RT_ARRAY:
                if (sq_getsize(v, -1)) {

                skip_size_check:
                    sq_getstackobj(v, -1, &val_obj);
                    for (const auto& vec_obj : recursion_vec) {
                        if (!memcmp(&val_obj._unVal, &vec_obj, sizeof(SQObjectValue))) {
                            printf("RECURSION DETECTED");
                            return;
                        }
                    }
                    recursion_vec.push_back(val_obj._unVal);

                    printf(val_type != _RT_ARRAY ? "{\n" : "[\n");

                    sq_pushnull(v);
                    while (SQ_SUCCEEDED(sq_next(v, -2))) {
                        //Key|value
                        // -2|-1
                        // SQObjectType key_type = sq_gettype(v, -2);
                        printf("%*s", depth, "");

                        if (val_type != _RT_ARRAY) {
                            const SQChar* key_str;
                            sq_getstring(v, -2, &key_str);
                            printf("\"%s\": ", key_str);
                        } else {
                            SQInteger idx;
                            sq_getinteger(v, -2, &idx);
                            printf("[%lld]: ", idx);
                        }

                        print_stack_top_value(recursion_vec, v, depth + 1);

                        printf(",\n");

                        sq_pop(v, 2);
                    }
                    sq_pop(v, 1);

                    recursion_vec.pop_back();

                    printf(val_type != _RT_ARRAY ? "%*s}" : "%*s]", depth - 1, "");
                } else {
                    printf(val_type != _RT_ARRAY ? "{}" : "[]");
                }
                break;
            case _RT_WEAKREF:
                sq_getweakrefval(v, -1);
                printf("(WeakRef) ");
                print_stack_top_value(recursion_vec, v, depth);
                sq_pop(v, 1);
                break;
            case _RT_OUTER:
                printf("Outer");
                break;
            default:
                std::unreachable();
        }
    }

    static void DumpSquirrelTable(HSQUIRRELVM vm, HSQOBJECT table) {
        sq_pushobject(vm, table);
        std::vector<SQObjectValue> recursion_vec;
        SQUtils::print_stack_top_value(recursion_vec, vm, 1);
        printf("\n");
        sq_pop(vm, 1);
    }
};
