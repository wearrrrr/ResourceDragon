#pragma once

#include <ArchiveFormat.h>
#include <unordered_map>

struct MPKEntry : Entry {
    u32 Id;
    int64_t Offset;
    bool Compressed;
    int64_t CompressedSize;
};

class MPKFormat : public ArchiveFormat {
    std::string_view tag = "NitroPlus.MPK";
    std::string_view description = "Nitro+ MPK Resource Archive";
    u32 sig = 0x4B504D;

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
    std::string_view GetTag() const override {
        return this->tag;
    }
    std::string_view GetDescription() const override {
        return this->description;
    }
};

class MPKArchive : public ArchiveBase {
    std::unordered_map<std::string, MPKEntry> entries;
    public:
        MPKArchive(const std::unordered_map<std::string, MPKEntry> &entries) {
            this->entries = entries;
        }
        EntryMapPtr GetEntries() override {
            EntryMapPtr entriesMap;
            for (auto &entry : entries) {
                entriesMap.insert({entry.first, &entry.second});
            }
            return entriesMap;
        }
        u8* OpenStream(const Entry *entry, u8 *buffer) override;
};
