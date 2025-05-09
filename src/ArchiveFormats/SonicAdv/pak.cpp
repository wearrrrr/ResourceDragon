#include "pak.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

ArchiveBase *SAPakFormat::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) {
    uint32_t file_count = Read<uint32_t>(buffer, 0x39);
    std::vector<Entry> entries(file_count);

    Seek(0x3D);
    
    for (uint32_t i = 0; i < file_count; i++) {
        uint32_t name_len = Read<uint32_t>(buffer);
        ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        name_len = Read<uint32_t>(buffer);
        entries[i].name = ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        entries[i].size = Read<uint32_t>(buffer);
        Advance(0x4);
    }

    for (uint32_t i = 0; i < file_count; i++) {

        std::vector<uint8_t> entry_data;
        entry_data.resize(entries[i].size);
        Read(entry_data.data(), buffer, entries[i].size);
        entries[i].data = std::move(entry_data);
    }

    return new SAPakArchive(entries);
};

const char *SAPakArchive::OpenStream(const Entry *entry, unsigned char *buffer) {
    return (const char *)entry->data.data();
};