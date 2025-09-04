#include "mpk.h"
#include <unordered_map>

static int constexpr MPKMaxPath = 224;

ArchiveBase *MPKFormat::TryOpen(u8 *buffer, u64 size, std::string file_name)
{
    if (!CanHandleFile(buffer, size, "")) return nullptr;

    // Move past byte magic.
    Seek(0x4);

    u32 FileCount;
    u16 MinorVersion;
    u16 MajorVersion;
    char name[MPKMaxPath];

    std::unordered_map<std::string, MPKEntry> entries;

    MinorVersion = Read<u16>(buffer);
    MajorVersion = Read<u16>(buffer);
    FileCount = Read<u16>(buffer);

    if (MinorVersion != 0 || MajorVersion != 2) {
        Logger::error("Unsupported MPK Version! Version found: {}.{}", MajorVersion, MinorVersion);
        return nullptr;
    }

    Seek(0x40);
    for (u32 i = 0; i < FileCount; i++) {
        u32 compression = Read<u32>(buffer);
        u32 id = Read<u32>(buffer);

        if (compression != 0 && compression != 1) {
            Logger::warn("Unknown compression type! {}", compression);
            Seek(0x100 - 8);
            continue;
        }

        MPKEntry entry;

        entry.Id = id;
        entry.Compressed = compression;
        entry.Offset = Read<u64>(buffer);
        entry.CompressedSize = Read<u64>(buffer);
        entry.size = Read<u64>(buffer);
        Read(name, buffer, MPKMaxPath);
        name[MPKMaxPath - 1] = '\0';
        entry.name = name;
        entries.insert({entry.name, entry});
    }

    return new MPKArchive(entries);
}

bool MPKFormat::CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const
{
    if (ReadMagic<u32>(buffer) == sig) {
        return true;
    }

    return false;
}

u8* MPKArchive::OpenStream(const Entry *entry, u8 *buffer)
{
    MPKEntry *mpkEntry = (MPKEntry*)entry;

    unsigned char *entry_offset = buffer + mpkEntry->Offset;

    u8* data = (u8*)malloc(mpkEntry->size);
    memcpy(data, entry_offset, mpkEntry->size);
    return data;
}
