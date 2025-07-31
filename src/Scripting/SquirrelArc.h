#include "SQUtils.hpp"
#include "ArchiveFormats/ArchiveFormat.h"

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
        // TODO: Convert entry back into something squirrel can work with so that we can actually pass relevant information to this function
        // Also, we need to return the return value of this function, obviously.
        SQUtils::call_squirrel_function_in_table(vm, archive_format_table, "OpenStream", nullptr, buffer);
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

  bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
  ArchiveBase* TryOpen(u8* buffer, u64 size, std::string file_name) override;

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
