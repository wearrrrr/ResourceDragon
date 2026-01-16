#pragma once

#include "SQUtils.h"
#include "SquirrelArc.h"
#include "RDSquirrelLib.h"

#include <cstdarg>
#include <cstdio>
#include <SDK/util/Logger.hpp>

static void squirrel_print(HSQUIRRELVM vm, const SQChar *str, ...) {
  va_list va;
  va_start(va, str);
  Logger::log([&]() {
    printf("[Squirrel] ");
    vprintf(str, va);
  });
  va_end(va);
}

#ifdef EMSCRIPTEN
#define SQ_RUNTIME_EXCEPTION_FORMAT "\t at %s (%s:%d)\n"
#else
#define SQ_RUNTIME_EXCEPTION_FORMAT "\t at %s (%s:%lld)\n"
#endif

static SQInteger squirrel_runtime_error(HSQUIRRELVM vm) {
  if (sq_gettop(vm) > 0) {
    const SQChar *error_msg;
    if (SQ_SUCCEEDED(sq_getstring(vm, 2, &error_msg))) {
      Logger::error("Squirrel runtime exception: \"{}\"", error_msg);
      SQStackInfos sqstack;
      for (SQInteger i = 1; SQ_SUCCEEDED(sq_stackinfos(vm, i, &sqstack)); ++i) {
        printf(sqstack.source ? SQ_RUNTIME_EXCEPTION_FORMAT : "\t at %s\n",
               sqstack.funcname ? sqstack.funcname : "Anonymous function",
               sqstack.source, sqstack.line);
      }
    }
  }
  return 0;
}

#ifdef EMSCRIPTEN
#define SQ_COMPILER_EXCEPTION_FORMAT "\t at %s:%d:%d: %s\n\n"
#else
#define SQ_COMPILER_EXCEPTION_FORMAT "\t at %s:%lld:%lld: %s\n\n"
#endif

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
      printf(SQ_COMPILER_EXCEPTION_FORMAT, src, line, col, desc);
    });
    sq_newclosure(vm, squirrel_runtime_error, 0);
    sq_seterrorhandler(vm);
    sq_setprintfunc(vm, squirrel_print, nullptr);
    sq_pushroottable(vm);

    RDSquirrelLib::RegisterAllFuncs(vm);

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
