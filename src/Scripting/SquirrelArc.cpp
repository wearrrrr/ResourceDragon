#include "SquirrelArc.h"

bool SquirrelArchiveFormat::CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const {
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

ArchiveBase* SquirrelArchiveFormat::TryOpen(u8* buffer, u64 size, std::string file_name) {
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

u8* SquirrelArchiveBase::OpenStream(const Entry *entry, u8 *buffer) {
    u8 *entryData = (u8*)malloc(entry->size);
    memcpy(entryData, buffer + entry->offset, entry->size);
    // TODO: Convert entry back into something squirrel can work with so that we can actually pass relevant information to this function
    // Also, we need to return the return value of this function, obviously.
    SQUtils::call_squirrel_function_in_table(vm, archive_format_table, "OpenStream", nullptr, buffer);
    return entryData;
};
