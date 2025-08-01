#pragma once

#include "SQUtils.hpp"
#include "squirrel_all.h"
#include "SquirrelArc.h"

#include <cstdio>
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
      printf("\t at %s:%d:%d: %s\n\n", src, line, col, desc);
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
