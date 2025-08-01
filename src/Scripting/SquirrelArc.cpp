#include "SquirrelArc.h"
#include "squirrel.h"

bool SquirrelArchiveFormat::CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const {
    SQBool result;

    if (!SQUtils::call_squirrel_function_in_table(vm, archive_format_table, "CanHandleFile", buffer, size, ext)) {
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
    HSQOBJECT result;

    sq_pushobject(vm, archive_format_table);

    if (!SQUtils::call_squirrel_function_in_table(vm, archive_format_table, "TryOpen", buffer, size, file_name)) {
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

    return new SquirrelArchiveBase(vm, archive_format_table, entries);
}

u8* SquirrelArchiveBase::OpenStream(const Entry* entry, u8* buffer) {
    sq_newtable(vm);
    sq_pushstring(vm, "name", -1);
    sq_pushstring(vm, entry->name.data(), entry->name.size());
    sq_rawset(vm, -3);

    sq_pushstring(vm, "size", -1);
    sq_pushinteger(vm, entry->size);
    sq_rawset(vm, -3);

    sq_pushstring(vm, "offset", -1);
    sq_pushinteger(vm, entry->offset);
    sq_rawset(vm, -3);

    HSQOBJECT entry_obj;
    sq_getstackobj(vm, -1, &entry_obj);
    sq_addref(vm, &entry_obj);

    sq_pop(vm, 1);

    if (SQ_SUCCESS(SQUtils::call_squirrel_function_in_table(vm, archive_format_table, "OpenStream", entry_obj, buffer))) {
        HSQOBJECT result;
        sq_getstackobj(vm, -1, &result);
        sq_addref(vm, &result);
        sq_pop(vm, 1);

        if (result._type == OT_ARRAY) {
            sq_pushobject(vm, result);
            SQInteger len = sq_getsize(vm, -1);

            std::vector<u8> entry_buf;
            entry_buf.reserve(len);

            for (SQInteger i = 0; i < len; ++i) {
                sq_pushinteger(vm, i);
                if (SQ_SUCCEEDED(sq_rawget(vm, -2))) {
                    SQInteger val;
                    if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &val))) {
                        entry_buf.push_back(static_cast<u8>(val));
                    }
                    sq_pop(vm, 1);
                }
            }

            sq_pop(vm, 1); // pop array

            sq_release(vm, &entry_obj);
            sq_release(vm, &result);

            u8* entryData = (u8*)malloc(entry_buf.size());
            memcpy(entryData, entry_buf.data(), entry_buf.size());
            return entryData;
        }
    }

    return nullptr;
}
