#pragma once

#include "SQUtils.hpp"
#include "squirrel.h"
#include "squirrel_all.h"

#include <ArchiveFormats/ArchiveFormat.h>
#include <util/Logger.h>

static void squirrel_print(HSQUIRRELVM vm, const SQChar* s, ...) {
  va_list va;
  va_start(va, s);
  Logger::log(s, va);
  va_end(va);
}

static SQInteger squirrel_runtime_error(HSQUIRRELVM vm) {
  if (sq_gettop(vm) > 0) {
    const SQChar *error_msg;
    if (SQ_SUCCEEDED(sq_getstring(vm, 2, &error_msg))) {
      Logger::error("Squirrel runtime exception: \"%s\" ", error_msg);
      SQStackInfos sqstack;
      for (SQInteger i = 1; SQ_SUCCEEDED(sq_stackinfos(vm, i, &sqstack)); ++i) {
        printf(*sqstack.source ? "\t at %s (%s:%lld)\n"
                               : "\t at %s (unknown)\n",
               sqstack.funcname ? sqstack.funcname : "Anonymous function",
               sqstack.source, sqstack.line);
      }
    }
  }
  return 0;
}

class SquirrelArchiveFormat : public ArchiveFormat {
  HSQUIRRELVM vm;
  HSQOBJECT table_ref;

public:
  SquirrelArchiveFormat(HSQUIRRELVM vm, const HSQOBJECT &table) : vm(vm) {
    sq_resetobject(&table_ref);
    table_ref = table;
  }

  ~SquirrelArchiveFormat() { sq_release(vm, &table_ref); }

  std::string_view GetTag() const override {
    return GetStringField("tag", "GETTAG_FAIL");
  }

  std::string_view GetDescription() const override {
    return GetStringField("description", "GETDESC_FAIL");
  }

  bool CanHandleFile(u8 *buffer, u64 size,
                     const std::string &ext) const override {
    return CallBoolFunction("canHandleFile", buffer, size, ext.c_str());
  }

  ArchiveBase *TryOpen(u8 *buffer, u64 size, std::string file_name) override {
    HSQOBJECT table =
        CallTableFunction("tryOpen", buffer, size, file_name.c_str());
    SQUtils::DumpSquirrelTable(vm, table);

    sq_pushobject(vm, table);
    sq_pushstring(vm, "entries", -1);

    EntryMap entries;

    if (SQ_SUCCEEDED(sq_get(vm, -2))) {
      if (sq_gettype(vm, -1) == OT_ARRAY) {
        // For each element in entries[]...
        for (SQInteger i = 0; i < sq_getsize(vm, -1); ++i) {
          sq_pushinteger(vm, i);
          if (SQ_SUCCEEDED(sq_get(vm, -2))) {

            if (sq_gettype(vm, -1) == OT_TABLE) {
              Entry entry;

              sq_pushstring(vm, "name", -1);
              if (SQ_SUCCEEDED(sq_get(vm, -2))) {
                const SQChar *str;
                if (SQ_SUCCEEDED(sq_getstring(vm, -1, &str))) {
                  entry.name = str;
                }
                sq_pop(vm, 1);
              }

              sq_pushstring(vm, "size", -1);
              if (SQ_SUCCEEDED(sq_get(vm, -2))) {
                SQInteger size;
                if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &size))) {
                  entry.size = size;
                }
                sq_pop(vm, 1);
              }

              entries.insert({entry.name, entry});
            }

            sq_pop(vm, 1);
          }
        }
      }
      sq_pop(vm, 1); // entries table
    };

    sq_pop(vm, 1); // table from tryOpen

    Logger::log("Entry 1 name: %s", entries.begin()->second.name.c_str());
    Logger::log("Entry 1 size: %d", entries.begin()->second.size);

    return nullptr;
  }

private:
  std::string_view GetStringField(const char *key, const char *fallback) const {
    sq_pushobject(vm, table_ref);
    sq_pushstring(vm, _SC(key), -1);
    if (SQ_SUCCESS(sq_get(vm, -2))) {
      const SQChar *val;
      if (SQ_SUCCESS(sq_getstring(vm, -1, &val))) {
        sq_pop(vm, 2);
        return val;
      }
    }
    sq_pop(vm, 1);
    return fallback;
  }

  HSQOBJECT CallTableFunction(const char *funcName, u8 *buffer, u64 size,
                              const char *file_name) const {
    HSQOBJECT result;
    result._type = OT_NULL;

    sq_pushobject(vm, table_ref);
    sq_pushstring(vm, _SC(funcName), -1);
    if (SQ_SUCCEEDED(sq_get(vm, -2))) {
      sq_pushobject(vm, table_ref);
      sq_pushuserpointer(vm, buffer);
      sq_pushinteger(vm, (SQInteger)size);
      sq_pushstring(vm, file_name, -1);

      if (SQ_SUCCEEDED(sq_call(vm, 4, SQTrue, SQTrue))) {
        if (sq_gettype(vm, -1) == OT_TABLE) {
          sq_getstackobj(vm, -1, &result);
          sq_addref(vm, &result);
        }
        sq_pop(vm, 2);
        return result;
      }
    }

    sq_pop(vm, 1);
    return result;
  }

  bool CallBoolFunction(const char *funcName, u8 *buffer, u64 size,
                        const char *ext) const {
    sq_pushobject(vm, table_ref);
    sq_pushstring(vm, SC(funcName), -1);
    if (SQ_SUCCESS(sq_get(vm, -2))) {
      sq_pushobject(vm, table_ref);
      sq_pushuserpointer(vm, buffer);
      sq_pushinteger(vm, size);
      sq_pushstring(vm, ext, -1);

      if (SQ_SUCCESS(sq_call(vm, 4, SQTrue, SQTrue))) {
        SQBool result;
        sq_getbool(vm, -1, &result);
        sq_pop(vm, 2);
        return result;
      }
    }
    sq_pop(vm, 1);
    return false;
  }

  void CallFunction(const char *funcName, u8 *buffer, u64 size,
                    const char *file_name) const {
    sq_pushobject(vm, table_ref);
    sq_pushstring(vm, _SC(funcName), -1);
    if (SQ_SUCCESS(sq_get(vm, -2))) {
      sq_pushobject(vm, table_ref);
      sq_pushuserpointer(vm, buffer);
      sq_pushinteger(vm, size);
      sq_pushstring(vm, file_name, -1);

      if (SQ_FAILED(sq_call(vm, 4, SQFalse, SQTrue))) {
        sqstd_printcallstack(vm);
      }
    }
    sq_pop(vm, 1);
  }
};

class ScriptManager {
  HSQUIRRELVM vm;

public:
  ScriptManager() {
    vm = sq_open(1024);
    if (!vm) {
      Logger::error("Failed to create Squirrel VM!");
      return;
    }
    sq_setcompilererrorhandler(vm, [](HSQUIRRELVM vm, const SQChar *desc,
                                      const SQChar *src, SQInteger line,
                                      SQInteger col) {
      puts("");
      Logger::error("Squirrel Compiler Exception!");
      printf("\t at %s:%lld:%lld: %s\n\n", src, line, col, desc);
    });
    sq_newclosure(vm, squirrel_runtime_error, 0);
    sq_seterrorhandler(vm);
    sq_setprintfunc(vm, squirrel_print, nullptr);
    sq_pushroottable(vm);

    sq_pushstring(vm, SC("read_bytes"), -1);
    sq_newclosure(vm, SQUtils::read_bytes, 0);
    sq_newslot(vm, -3, SQFalse);

    sqstd_register_iolib(vm);
    sqstd_register_mathlib(vm);
    sqstd_register_stringlib(vm);
    sqstd_register_bloblib(vm);
    sqstd_register_systemlib(vm);

    sq_pop(vm, 1);
  }
  ~ScriptManager() {
    Logger::log("Closing Squirrel...");
    sq_close(vm);
  }

  bool LoadFile(std::string path);
  SquirrelArchiveFormat *Register();
};
