
#include "ScriptManager.h"

void ScriptManager::LoadFile(std::string path) {
    Logger::log("[Squirrel] Loading %s...", path.c_str());

    sq_pushroottable(vm);

    if (SQ_FAILED(sqstd_dofile(vm, path.c_str(), SQFalse, SQTrue))) {
        Logger::error("[Squirrel] Failed to load+run script: %s", path.c_str());
        sqstd_printcallstack(vm);
    }

    sq_pop(vm, 1);
}

SquirrelArchiveFormat* ScriptManager::Register() {
    sq_pushroottable(vm);

    sq_pushstring(vm, _SC("archive_format"), -1);
    if (SQ_FAILED(sq_get(vm, -2))) {
        Logger::error("[Squirrel] 'archive_format' not found in script.");
        sq_pop(vm, 1);
        return nullptr;
    }

    if (sq_gettype(vm, -1) != OT_TABLE) {
        Logger::error("[Squirrel] 'archive_format' is not a table.");
        sq_pop(vm, 2);
        return nullptr;
    }

    HSQOBJECT table_obj;
    sq_getstackobj(vm, -1, &table_obj);
    sq_addref(vm, &table_obj);

    sq_pop(vm, 2);

    return new SquirrelArchiveFormat(vm, table_obj);
}
