#include "pfs.h"
#include "../../sha1.h"

#include <unordered_map>
#include <algorithm>


ArchiveBase *PFSFormat::TryOpen(u8 *buffer, u64 size, std::string file_name)
{
    if (!CanHandleFile(buffer, size, "")) return nullptr;

    // - '0' converts to ascii representation
    u8 version = Read<u8>(buffer, 2) - '0';

    switch (version) {
        case 6:
        case 8:
            return OpenPF(buffer, size, version);
        default:
            return nullptr;
    }

    return nullptr;
}

ArchiveBase *PFSFormat::OpenPF(u8 *buffer, u64 size, u8 version) {
    u32 index_size = Read<u32>(buffer, 3);
    u32 file_count = Read<u32>(buffer, 7);

    if (!IsSaneFileCount(file_count)) {
        Logger::error("File count is %d. This is way too high!", file_count);
        return nullptr;
    }
    if (index_size > size) {
        Logger::error("Index size is greater than the file size! This is invalid.");
        return nullptr;
    }
    u8 *index_buf = (u8*)malloc(index_size);
    Seek(0x7);

    Read(index_buf, buffer, index_size);

    // std::ofstream outFile("index_buf", std::ios::binary);
    // outFile.write((const char*)index_buf, index_size);
    // outFile.close();

    std::unordered_map<std::string, Entry> entries;

    u32 index_offset = 4;
    for (u32 i = 0; i < file_count; ++i) {
        if (index_offset + 4 > index_size) {
            Logger::error("Unexpected end of index when reading name length.");
            break;
        }

        u32 name_length = Read<u32>(index_buf, index_offset);
        if (index_offset + 4 + name_length + 8 + 8 > index_size) {
            Logger::error("Index overrun when reading entry %d", i);
            break;
        }

        std::string name = ReadStringWithLength(index_buf + index_offset + 4, name_length);
        index_offset += name_length + 8;

        u32 offset = Read<u32>(index_buf, index_offset);
        u32 size = Read<u32>(index_buf, index_offset + 4);
        index_offset += 8;

        Entry entry;
        entry.name = name;
        entry.offset = offset;
        entry.size = size;

        entries.insert({name, entry});
    }

    if (version != 8 && version != 9 && version != 4 && version != 5)
        return new PFSArchive(entries);

    Chocobo1::SHA1 sha;

    sha.addData(index_buf, index_size);
    auto key = sha.finalize();

    free(index_buf);

    return new PFSArchive(this, entries, key.toVector());
}

bool PFSFormat::CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const
{
    if (ext != "" && std::find(extensions.begin(), extensions.end(), ext) == extensions.end()) {
        return false;
    }

    if (ReadMagic<u16>(buffer) == PackUInt16('p', 'f')) {
        return true;
    }

    return false;
}

const char* PFSArchive::OpenStream(const Entry *entry, u8 *buffer) {
    u8* output = (u8*)malloc(entry->size);
    memcpy(output, buffer + entry->offset, entry->size);

    pfs_fmt->buffer_position += entry->size;
    for (u32 i = 0; i < entry->size; ++i)
    {
        output[i] ^= key[i % key.size()];
    }

    return (const char*)(output);
}
