#include "pfs.h"
#include <fstream>

ArchiveBase *PFSFormat::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name)
{
    if (!CanHandleFile(buffer, size, "")) return nullptr;

    // - '0' changes it to 8 instead of 0x38 (the ascii version of 8)
    uint8_t version = Read<uint8_t>(buffer, 2) - '0';

    switch (version) {
        case 6:
        case 8:
            return OpenPF(buffer, size, version);
        default:
            return nullptr;
    }

    return nullptr;
}

ArchiveBase *PFSFormat::OpenPF(unsigned char *buffer, uint32_t size, uint8_t version) {
    uint32_t index_size = Read<uint32_t>(buffer, 3);
    int32_t file_count = Read<int32_t>(buffer, 7);

    Logger::log("Index size: %u", index_size);
    Logger::log("File count: %d", file_count);

    if (!IsSaneFileCount(file_count)) {
        Logger::error("File count is %d. This is way too high!", file_count);
        return nullptr;
    }
    if (index_size > size) {
        Logger::error("Index size is greater than the file size! This is invalid.");
        return nullptr;
    }
    unsigned char *index_buf = (unsigned char*)malloc(index_size);
    Seek(7);

    Read(index_buf, buffer, index_size);

    // std::ofstream outFile("index_buf", std::ios::binary);
    // outFile.write((const char*)index_buf, index_size);
    // outFile.close();
    
    std::vector<Entry> entries;
    
    int32_t index_offset = 4;
    for (int i = 0; i < 5; ++i) {
        if (index_offset + 4 > index_size) {
            Logger::error("Unexpected end of index when reading name length.");
            break;
        }
    
        int32_t name_length = Read<int32_t>(index_buf, index_offset);
        if (index_offset + 4 + name_length + 8 + 8 > index_size) {
            Logger::error("Index overrun when reading entry %d", i);
            break;
        }
    
        std::string name = ReadStringWithLength(index_buf + index_offset + 4, name_length);
        index_offset += name_length + 8;
    
        uint32_t offset = Read<uint32_t>(index_buf, index_offset);
        uint32_t size = Read<uint32_t>(index_buf, index_offset + 4);
        index_offset += 8;
    
        Entry entry;
        entry.name = name;
        entry.offset = offset;
        entry.size = size;
     
        entries.push_back(entry);
    }

    if (version != 8 && version != 9 && version != 4 && version != 5)
        return new PFSArchive(entries);

    Chocobo1::SHA1 sha;

    sha.addData(index_buf, index_size);
    auto key = sha.finalize();

    free(index_buf);

    return new PFSArchive(entries, key.toVector());
}

bool PFSFormat::CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const
{
    if (ext != "" && std::find(extensions.begin(), extensions.end(), ext) == extensions.end()) {
        return false;
    }

    if (*based_pointer<uint16_t>(buffer, 0) == PackUInt16('p', 'f')) {
        return true;
    }

    return false;
}

const char* PFSArchive::OpenStream(const Entry *entry, unsigned char *buffer) {
    unsigned char* output = new unsigned char[entry->size];
    memcpy(output, buffer + entry->offset, entry->size);

    if (!key.empty()) {
        size_t base_offset = entry->offset % key.size();
        for (size_t i = 0; i < entry->size; ++i) {
            size_t stream_pos = i;
            output[i] ^= key[(base_offset + stream_pos) % key.size()];
        }
    }

    return (const char*)(output);
}