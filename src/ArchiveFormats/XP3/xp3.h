#pragma once

#include <string>
#include <zlib.h>
#include <unordered_map>

#include <ArchiveFormat.h>

enum XP3HeaderType {
    XP3_HEADER_UNPACKED = 0,
    XP3_HEADER_PACKED = 1,
};

class XP3Format : public ArchiveFormat {
    public:
        std::string tag = "XP3";
        std::string description = "XP3 Archive Format";

        u32 sig = 0x0d335058; // "XP3\0D"

        std::vector<std::string> extensions = {"xp3", "exe"};

        static constexpr u8 xp3_header[] = {
            0x58, 0x50, 0x33, 0x0d, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01
        };

        ArchiveBase *TryOpen(u8 *buffer, u64 size, std::string file_name) override;

        std::string GetTag() const override {
            return this->tag;
        };

        bool CanHandleFile(u8 *buffer, u64 size, const std::string &_ext) const override {
            return (size > 0x10 && memcmp(buffer, xp3_header, sizeof(xp3_header)) == 0);
        };
};

class XP3Archive : public ArchiveBase {
    public:
        std::unordered_map<std::string, Entry> entries;
        XP3Archive(std::unordered_map<std::string, Entry> entries) {
            this->entries = entries;
        };

        // std::vector<Entry*> GetEntries() override {
        //     std::vector<Entry*> basePtrs;
        //     for (auto& entry : entries)
        //         basePtrs.push_back(&entry);
        //     return basePtrs;
        // }
        std::unordered_map<std::string, Entry*> GetEntries() override {
            std::unordered_map<std::string, Entry*> entries;
            for (auto& entry : this->entries)
                entries[entry.first] = &entry.second;
            return entries;
        }
        u8* OpenStream(const Entry *entry, u8 *buffer) override;
};
