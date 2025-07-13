#pragma once

#include <unordered_map>

#include <ArchiveFormat.h>
#include <Entry.h>

class HSPArchive : public ArchiveFormat {
    public:
        std::string_view tag = "HSP";
        std::string_view description = "Hot Soup Processor 3 Resource Archive";

        u32 DefaultKey = 0xAC52AE58;

        std::vector<std::string> extensions = {"exe", "dpm", "bin", "dat"};

        HSPArchive() {
            sig = PackUInt('D', 'P', 'M', 'X');
        };


        u32 FindExeKey(ExeFile *exe, u32 dpmx_offset);

        ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
        bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
        std::string_view GetTag() const override {
            return this->tag;
        }
        std::string_view GetDescription() const override {
            return this->description;
        }
};

class DPMArchive : public ArchiveBase {
    public:
        EntryMap entries;

        u32 arc_key;
        size_t dpm_size;
        u8 seed_1;
        u8 seed_2;
        DPMArchive() {
            seed_1 = 0xAA;
            seed_2 = 0x55;
        };
        DPMArchive(const EntryMap &entries, u32 arc_key, size_t dpm_size) {
            seed_1 = ((((arc_key >> 16) & 0xFF) * (arc_key & 0xFF) / 3) ^ dpm_size);
            seed_2 = ((((arc_key >> 8)  & 0xFF) * ((arc_key >> 24) & 0xFF) / 5) ^ dpm_size ^ 0xAA);
            this->entries = entries;
            this->arc_key = arc_key;
            this->dpm_size = dpm_size;
        };
        u8* DecryptEntry(u8 *data, u32 data_size, u32 entry_key) {
            // TODO: These values seem to swap between games? Maybe different versions of the engine..?
            u8 *buffer = (u8*)malloc(data_size);
            memcpy(buffer, data, data_size);

            u8 s1 = 0x55;
            u8 s2 = 0xAA;
            s1 = (seed_1 + ((entry_key >> 16) ^ (entry_key + s1)));
            s2 = (seed_2 + ((entry_key >> 24) ^ ((entry_key >> 8) + s2)));
            u8 val = 0;
            for (u32 i = 0; i < data_size; i++) {
                val += (s1 ^ (buffer[i] - s2));
                buffer[i] = val;
            }
            return buffer;
        };
        // std::vector<Entry*> GetEntries() override {
        //     std::vector<Entry*> basePtrs;
        //     for (auto& entry : entries)
        //         basePtrs.push_back(&entry);
        //     return basePtrs;
        // }
        EntryMapPtr GetEntries() override {
            EntryMapPtr entriesMap;
            for (auto &entry : entries) {
                entriesMap.insert({entry.first, &entry.second});
            }
            return entriesMap;
        }
        u8* OpenStream(const Entry *entry, u8 *buffer) override;
};
