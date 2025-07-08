#include "pak.h"
#include <unordered_map>

ArchiveBase *SAPakFormat::TryOpen(u8 *buffer, u64 size, std::string file_name) {
    u32 file_count = Read<u32>(buffer, 0x39);
    EntryMap entries;
    std::vector<std::string> names;

    Seek(0x3D);

    for (u32 i = 0; i < file_count; i++) {
        Entry entry;
        u32 name_len = Read<u32>(buffer);
        ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        name_len = Read<u32>(buffer);
        entry.name = ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        entry.size = Read<u32>(buffer);
        entries.insert({entry.name, entry});
        names.push_back(entry.name);
        Advance(0x4);
    }

    for (u32 i = 0; i < file_count; i++) {
        std::vector<u8> entry_data;
        entry_data.resize(entries.find(names[i])->second.size);
        Read(entry_data.data(), buffer, entries[names[i]].size);
        entries[names[i]].data = std::move(entry_data);
    }

    return new SAPakArchive(entries);
};

u8* SAPakArchive::OpenStream(const Entry *entry, u8 *buffer) {
    return (u8*)entry->data.data();
};
