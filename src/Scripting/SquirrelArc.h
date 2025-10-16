#include "SQUtils.h"
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

    u8* OpenStream(const Entry *entry, u8 *buffer) override;
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

    char *tag;
    char *description;

  SquirrelArchiveFormat(HSQUIRRELVM vm, const HSQOBJECT &table) : vm(vm) {
    sq_resetobject(&archive_format_table);
    archive_format_table = table;
    this->tag = (char*)malloc(128);
    this->description = (char*)malloc(512);
  }

  ~SquirrelArchiveFormat() {
      sq_release(vm, &archive_format_table);
      free(this->tag);
      free(this->description);
  }

  const char* GetTag() const override {
      snprintf(this->tag, 128, "%s", GetStringField("tag", "TAG_FAIL").c_str());
      return (const char*)tag;
  }

  const char* GetDescription() const override {
    snprintf(this->description, 512, "%s", GetStringField("description", "GETDESC_FAIL").c_str());
    return (const char*)description;
  }

  bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
  ArchiveBase* TryOpen(u8* buffer, u64 size, std::string file_name) override;

private:
  std::string GetStringField(const char *key, const char *fallback) const {
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
