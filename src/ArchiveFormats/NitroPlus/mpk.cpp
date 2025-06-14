#include "mpk.h"
#include <unordered_map>

static int constexpr MPKMaxPath = 224;

ArchiveBase *MPKFormat::TryOpen(uint8_t *buffer, uint64_t size, std::string file_name)
{
    if (!CanHandleFile(buffer, size, "")) return nullptr;

    // Move past byte magic.
    Seek(0x4);

    uint32_t FileCount;
    uint16_t MinorVersion;
    uint16_t MajorVersion;
    char name[MPKMaxPath];

    std::unordered_map<std::string, MPKEntry> entries;

    MinorVersion = Read<uint16_t>(buffer);
    MajorVersion = Read<uint16_t>(buffer);
    FileCount = Read<uint16_t>(buffer);

    if (MinorVersion != 0 || MajorVersion != 2) {
        Logger::error("Unsupported MPK Version! Version found: %u.%u", MajorVersion, MinorVersion);
        return nullptr;
    }

    Seek(0x40);
    for (uint32_t i = 0; i < FileCount; i++) {
        uint32_t compression = Read<uint32_t>(buffer);
        uint32_t id = Read<uint32_t>(buffer);

        if (compression != 0 && compression != 1) {
            Logger::warn("Unknown compression type! %x", compression);
            Seek(0x100 - 8);
            continue;
        }

        MPKEntry entry;

        entry.Id = id;
        entry.Compressed = compression;
        entry.Offset = Read<uint64_t>(buffer);
        entry.CompressedSize = Read<uint64_t>(buffer);
        entry.size = Read<uint64_t>(buffer);
        Read(name, buffer, MPKMaxPath);
        name[MPKMaxPath - 1] = '\0';
        entry.name = name;
        entries.insert({entry.name, entry});
    }

    return new MPKArchive(entries);
}

bool MPKFormat::CanHandleFile(uint8_t *buffer, uint64_t size, const std::string &ext) const
{
    if (ReadMagic<uint32_t>(buffer) == sig) {
        return true;
    }

    return false;
}

const char *MPKArchive::OpenStream(const Entry *entry, uint8_t *buffer)
{
    MPKEntry *mpkEntry = (MPKEntry*)entry;

    unsigned char *entry_offset = buffer + mpkEntry->Offset;

    char *data = new char[mpkEntry->size];
    memcpy(data, entry_offset, mpkEntry->size);
    return data;
}
