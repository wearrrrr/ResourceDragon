#pragma once

#include "SQUtils.hpp"
#include "squirrel_all.h"

#include <ArchiveFormats/ArchiveFormat.h>
#include <cstdio>
#include <cstring>
#include <util/Logger.h>

static void squirrel_print(HSQUIRRELVM vm, const SQChar *str, ...) {
  va_list va;
  va_start(va, str);
  Logger::log([&]() {
    printf("[Squirrel] ");
    vprintf(str, va);
  });
  va_end(va);
}

static SQInteger squirrel_runtime_error(HSQUIRRELVM vm) {
  if (sq_gettop(vm) > 0) {
    const SQChar *error_msg;
    if (SQ_SUCCEEDED(sq_getstring(vm, 2, &error_msg))) {
      Logger::error("Squirrel runtime exception: \"%s\" ", error_msg);
      SQStackInfos sqstack;
      for (SQInteger i = 1; SQ_SUCCEEDED(sq_stackinfos(vm, i, &sqstack)); ++i) {
        printf(sqstack.source ? "\t at %s (%s:%lld)\n" : "\t at %s\n",
               sqstack.funcname ? sqstack.funcname : "Anonymous function",
               sqstack.source, sqstack.line);
      }
    }
  }
  return 0;
}

class SquirrelArchiveBase : public ArchiveBase {
  public:
    HSQUIRRELVM vm;
    HSQOBJECT archive_format_table;
    EntryMap entries;

    SquirrelArchiveBase(HSQUIRRELVM vm, HSQOBJECT table, EntryMap entries) {
        this->vm = vm;
        this->entries = entries;
        this->archive_format_table = table;
    }

    u8* OpenStream(const Entry *entry, u8 *buffer) override {
        u8 *entryData = (u8*)malloc(entry->size);
        memcpy(entryData, buffer + entry->offset, entry->size);
        // SQUtils::CallTableFunction(vm, archive_format_table, "OpenStream", buffer, 0, nullptr);
        return entryData;
    };
    EntryMapPtr GetEntries() override {
        EntryMapPtr entries;
        for (auto& entry : this->entries)
            entries[entry.first] = &entry.second;
        return entries;
    }
};

class SquirrelArchiveFormat : public ArchiveFormat {
  HSQUIRRELVM vm;
  HSQOBJECT archive_format_table;

public:
  SquirrelArchiveFormat(HSQUIRRELVM vm, const HSQOBJECT &table) : vm(vm) {
    sq_resetobject(&archive_format_table);
    archive_format_table = table;
  }

  ~SquirrelArchiveFormat() {
      sq_release(vm, &archive_format_table);
  }

  std::string_view GetTag() const override {
    return GetStringField("tag", "GETTAG_FAIL");
  }

  std::string_view GetDescription() const override {
    return GetStringField("description", "GETDESC_FAIL");
  }

  bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override {
      const char *funcName = "CanHandleFile";
      SQBool result;

      if (!SQUtils::call_squirrel_function_in_table(vm, archive_format_table, funcName, buffer, size, ext)) {
          return false;
      }

      if (SQ_FAILED(sq_getbool(vm, -1, &result))) {
          sq_pop(vm, 1);
          return false;
      }

      sq_pop(vm, 1);
      return result;
  }

  ArchiveBase* TryOpen(u8* buffer, u64 size, std::string file_name) override {
      const char *funcName = "TryOpen";
      HSQOBJECT result;

      sq_pushobject(vm, archive_format_table);

      if (!SQUtils::call_squirrel_function_in_table(vm, archive_format_table, funcName, buffer, size, file_name)) {
          sq_pop(vm, 1);
          return nullptr;
      }
      if (sq_gettype(vm, -1) == OT_TABLE) {
          sq_getstackobj(vm, -1, &result);
          sq_addref(vm, &result);
      } else {
          sq_pop(vm, 1);
          sq_pop(vm, 1);
          return nullptr;
      }
      sq_pop(vm, 1);

      SQUtils::DumpSquirrelTable(vm, result);

      sq_pushobject(vm, result);
      sq_pushstring(vm, "entries", -1);

      if (SQ_FAILED(sq_get(vm, -2))) {
          sq_pop(vm, 2);
          sq_pop(vm, 1);
          return nullptr;
      }

      EntryMap entries;

      if (sq_gettype(vm, -1) == OT_ARRAY) {
          SQInteger size = sq_getsize(vm, -1);
          for (SQInteger i = 0; i < size; ++i) {
              sq_pushinteger(vm, i);
              if (SQ_SUCCEEDED(sq_get(vm, -2))) {
                  if (sq_gettype(vm, -1) == OT_TABLE) {
                      Entry entry;
                      entry.name = SQUtils::GetStringFromStack(vm, "name");
                      entry.size = SQUtils::GetIntFromStack(vm, "size");
                      entry.offset = SQUtils::GetIntFromStack(vm, "offset");
                      entries.insert({entry.name, entry});
                  }
                  sq_pop(vm, 1);
              }
          }
      }

      sq_pop(vm, 2); // pop entries array and result table

      Logger::log("Entry 1 name: %s", entries.begin()->second.name.c_str());
      Logger::log("Entry 1 size: %d", entries.begin()->second.size);
      Logger::log("Entry 1 offset: %d", entries.begin()->second.offset);

      return new SquirrelArchiveBase(vm, archive_format_table, entries);
  }

private:
  std::string_view GetStringField(const char *key, const char *fallback) const {
    sq_pushobject(vm, archive_format_table);
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
    sq_setcompilererrorhandler(vm, [](HSQUIRRELVM vm, const SQChar *desc, const SQChar *src, SQInteger line, SQInteger col) {
      Logger::error("\nSquirrel Compiler Exception!");
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
