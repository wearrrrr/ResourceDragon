#include <ArchiveFormat.h>
#include "../../vendored/thlib/include/thlib.h"

static int findPbg3Entry(ThArchive *dat, const char *entry) {
    for (unsigned idx = 0; idx < dat->entries_count; ++idx) {
        if (strcmp(entry, dat->entries[idx].name) == 0)
            return idx;
    }
    return -1;
}

class THDAT : public ArchiveFormat {
    std::string tag = "Touhou.PGB3";
    std::string description = "Archive format for mainline Touhou games";
    u32 sig = PackUInt32('P', 'B', 'G', '3');

    std::vector<std::string> extensions = {".dat", ".DAT"};

    ArchiveBase *TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    ArchiveBase *TryOpenTH06(u8 *buffer, u64 size, std::string file_name);

    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override {
        if (ext == "dat" || ext == "DAT")
            return Read<u32>(buffer, 0) == sig;

        return false;
    };
    std::string GetTag() const override {
        return this->tag;
    };
    std::string GetDescription() const override {
        return this->description;
    }
};

class THDATArchive : public ArchiveBase {
    public:
    ThArchive dat;
    std::unordered_map<std::string, ThEntry> entries;

    THDATArchive(const ThArchive &dat, std::unordered_map<std::string, ThEntry> entries) {
        this->dat = dat;
        this->entries = entries;
    }

    EntryMapPtr GetEntries() override {
        EntryMapPtr entry_map;

        for (auto& [name, datEntry] : entries) {
            int index = findPbg3Entry(&dat, name.c_str());
            if (index == -1) continue;

            void* raw = thGetEntry(&dat, index);
            if (!raw) continue;

            Entry* rdEntry = new Entry{
                .name = name,
                .size = datEntry.uncompressed_size,
                .packedSize = datEntry.compressed_size
            };
            entry_map.emplace(name, rdEntry);
        }

        return entry_map;
    }

    u8* OpenStream(const Entry* entry, u8* buffer) override {
        auto idx = findPbg3Entry(&dat, entry->name.c_str());

        auto data = thGetEntry(&dat, idx);

        return (u8*)data;
    }
};
