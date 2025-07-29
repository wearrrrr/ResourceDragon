
#include "ScriptManager.h"

bool ScriptManager::LoadFile(std::string path) {
    Logger::log("Loading %s...", path.c_str());

    sq_pushroottable(vm);

    if (SQ_FAILED(sqstd_dofile(vm, path.c_str(), SQFalse, SQTrue))) {
        Logger::error("Failed to load script: %s", path.c_str());
        sqstd_printcallstack(vm);
        return false;
    }

    sq_pop(vm, 1);
    return true;
}

SquirrelArchiveFormat* ScriptManager::Register() {
    sq_pushroottable(vm);

    sq_pushstring(vm, _SC("archive_format"), -1);
    if (SQ_FAILED(sq_get(vm, -2))) {
        Logger::error("[Squirrel] 'archive_format' table not found in script!");
        sq_pop(vm, 1);
        return nullptr;
    }

    if (sq_gettype(vm, -1) != OT_TABLE) {
        Logger::error("[Squirrel] 'archive_format' exists but is not a table!");
        sq_pop(vm, 2);
        return nullptr;
    }

    HSQOBJECT table_obj;
    sq_getstackobj(vm, -1, &table_obj);
    sq_addref(vm, &table_obj);

    sq_pop(vm, 2);

    return new SquirrelArchiveFormat(vm, table_obj);
}
