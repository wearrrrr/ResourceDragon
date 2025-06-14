#include "pak.h"
#include <unordered_map>

ArchiveBase *SAPakFormat::TryOpen(uint8_t *buffer, uint64_t size, std::string file_name) {
    uint32_t file_count = Read<uint32_t>(buffer, 0x39);
    std::unordered_map<std::string, Entry> entries;
    std::vector<std::string> names;

    Seek(0x3D);

    for (uint32_t i = 0; i < file_count; i++) {
        Entry entry;
        uint32_t name_len = Read<uint32_t>(buffer);
        ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        name_len = Read<uint32_t>(buffer);
        // entries[i].name = ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        // entries[i].size = Read<uint32_t>(buffer);
        entry.name = ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        entry.size = Read<uint32_t>(buffer);
        entries.insert({entry.name, entry});
        names.push_back(entry.name);
        Advance(0x4);
    }

    for (uint32_t i = 0; i < file_count; i++) {
        std::vector<uint8_t> entry_data;
        entry_data.resize(entries.find(names[i])->second.size);
        Read(entry_data.data(), buffer, entries[names[i]].size);
        entries[names[i]].data = std::move(entry_data);
    }

    return new SAPakArchive(entries);
};

const char *SAPakArchive::OpenStream(const Entry *entry, uint8_t *buffer) {
    return (const char *)entry->data.data();
};
