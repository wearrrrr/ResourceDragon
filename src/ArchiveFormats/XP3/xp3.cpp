#include "xp3.h"
#include "Entry.h"
#include "XP3/Crypt/Crypt.h"
#include "../../util/Text.h"

static XP3Crypt *ALG_DEFAULT = new NoCrypt();

ArchiveBase *XP3Format::TryOpen(unsigned char *buffer, uint64_t size, std::string file_name) {
    int64_t base_offset = 0;

    if (!CanHandleFile(buffer, size, "")) {
        Logger::error("XP3: Invalid file signature!");
        return nullptr;
    }

    int64_t dir_offset = base_offset + Read<uint64_t>(buffer, base_offset + 0x0B);

    if (dir_offset < 0x13 || dir_offset >= size) return nullptr;

    if (Read<uint32_t>(buffer, dir_offset) == 0x80) {
        dir_offset = base_offset + Read<int64_t>(buffer, dir_offset + 0x9);
        if (dir_offset < 0x13) return nullptr;
    }

    int8_t header_type = Read<int8_t>(buffer, dir_offset);
    if (header_type != XP3_HEADER_UNPACKED && header_type != XP3_HEADER_PACKED) {
        Logger::error("XP3: Header type is invalid!");
        return nullptr;
    }
    Logger::log("XP3 Header type: %s", header_type == XP3_HEADER_UNPACKED ? "Unpacked" : "Packed");

    std::vector<uint8_t> header_stream;
    if (header_type == XP3_HEADER_UNPACKED) {
        int64_t header_size = Read<int64_t>(buffer, dir_offset + 0x1);
        if ((uint64_t)header_size > UINT64_MAX) {
            Logger::error("XP3: Header size is invalid!");
            return nullptr;
        }
        header_stream.resize(header_size);
        memcpy(header_stream.data(), buffer + dir_offset + 0x9, (uint32_t)header_size);
    } else {
        int64_t packed_size = Read<int64_t>(buffer, dir_offset + 0x1);
        if ((uint64_t)packed_size > UINT64_MAX) {
            Logger::error("XP3: Packed size is invalid!");
            return nullptr;
        }
        int64_t header_size = Read<int64_t>(buffer, dir_offset + 0x9);
        const uint8_t* compressed_data = buffer + dir_offset + 0x11;
        header_stream.resize(header_size);
        uLongf decompressed_size = (uLongf)(header_size);
        uLongf compressed_size = (uLongf)(packed_size);
        if (uncompress(header_stream.data(), &decompressed_size, compressed_data, compressed_size) != Z_OK) {
            Logger::error("XP3: Failed to decompress header!");
            return nullptr;
        }
        if (decompressed_size != (uLongf)header_size) {
            Logger::error("XP3: Decompressed size does not match header size!");
            return nullptr;
        }
    }

    std::unordered_map<std::string, Entry> dir;
    dir_offset = 0;

    BinaryReader header(header_stream);

    while (header.peek().has_value()) {
        uint32_t entry_signature = header.read<uint32_t>();
        int64_t entry_size = header.read<int64_t>();
        if (entry_size < 0) {
            Logger::error("XP3: Entry size is invalid!");
            return nullptr;
        }
        dir_offset += 12 + entry_size;

        if (entry_signature == PackUInt32('F', 'i', 'l', 'e')) {
            Entry entry = {};
            while (entry_size > 0) {
                uint32_t section = header.read<uint32_t>();
                int64_t section_size = header.read<int64_t>();
                entry_size -= 12;
                if (section_size > entry_size)
                {
                    if (section != 0x6f666e69)
                        break;
                    section_size = entry_size;
                }
                entry_size -= section_size;
                int64_t next_section_pos = header.position + section_size;
                switch (section) {
                    case PackUInt32('i', 'n', 'f', 'o'): {
                        if (entry.size != 0 || !entry.name.empty()) {
                            goto NextEntry;
                        }
                        entry.isEncrypted = header.read<uint32_t>() != 0;
                        int64_t file_size = header.read<int64_t>();
                        int64_t packed_size = header.read<int64_t>();
                        if (file_size >= UINT32_MAX || packed_size > UINT32_MAX || packed_size > size)
                        {
                            goto NextEntry;
                        }
                        entry.isPacked   = file_size != packed_size;
                        entry.packedSize = packed_size;
                        entry.size       = file_size;

                        if (entry.isEncrypted) {
                            entry.crypt = ALG_DEFAULT;
                        } else {
                            entry.crypt = new NoCrypt();
                        }

                        auto name = TextConverter::UTF16ToUTF8(entry.crypt->ReadName(header));
                        if (name.empty()) {
                            Logger::error("XP3: Failed to read entry name!");
                            goto NextEntry;
                        }
                        if (entry.crypt->ObfuscatedIndex)
                        {
                            goto NextEntry;
                        }
                        if (name.size() > 0x100)
                        {
                            goto NextEntry;
                        }
                        entry.name = name;
                        entry.isEncrypted = !(entry.crypt->GetCryptName() == "NoCrypt");

                        break;
                    }
                    case PackUInt32('s', 'e', 'g', 'm'): {
                        int32_t segment_count = section_size / 0x1C;
                        if (segment_count > 0) {
                            for (int i = 0; i < segment_count; ++i) {
                                bool compressed = header.read<int32_t>() != 0;
                                int64_t segment_offset = base_offset + header.read<int64_t>();
                                int64_t segment_size = header.read<int64_t>();
                                int64_t segment_packed_size = header.read<int64_t>();
                                if (segment_offset > size || segment_packed_size > size)
                                {
                                    goto NextEntry;
                                }
                                Segment segment = {
                                    .IsCompressed = compressed,
                                    .Offset = segment_offset,
                                    .Size = segment_size,
                                    .PackedSize = segment_packed_size
                                };
                                entry.segments.push_back(segment);
                            }
                            entry.offset = entry.segments.begin()->Offset;
                        }
                    }
                    case PackUInt32('a', 'd', 'l', 'r'): {
                        if (section_size == 4) {
                            entry.hash = header.read<uint32_t>();
                        }
                    }
                    default: // unknown section
                        break;
                }
                header.position = next_section_pos;
            }
            if (!entry.name.empty() && entry.segments.size() > 0) {
                dir.insert({entry.name, entry});
            }
        } else if ((entry_signature >> 24) == 0x3A) {
            Logger::log("yuz/sen/dls entry found! I don't know how to handle these!!");
        } else if (entry_size > 7) {
            // uint32_t hash header.read<uint32_t>();
            header.read<uint32_t>();
            int16_t name_size = header.read<int16_t>();
            if (name_size > 0) {
                entry_size -= 6;
                if (name_size * 2 <= entry_size)
                {
                    // TODO: filemap entries
                }
            }
        }
        NextEntry:
            header.position = dir_offset;
    }
    return new XP3Archive(dir);
}

static std::vector<uint8_t> stream;
static std::vector<uint8_t> decrypted;

const char *XP3Archive::OpenStream(const Entry *entry, unsigned char *buffer)
{
    stream.clear();
    if (entry->segments.size() == 1 && !entry->isEncrypted) {
        Segment segment = entry->segments.at(0);
        if (segment.IsCompressed) {
            stream.resize(segment.Size);
            uLongf decompressed_size = (uLongf)(segment.Size);
            uLongf compressed_size = (uLongf)(segment.PackedSize);
            if (uncompress(stream.data(), &decompressed_size, buffer + segment.Offset, compressed_size) != Z_OK) {
                Logger::error("XP3: Failed to decompress entry!");
                return nullptr;
            }
            if (decompressed_size != (uLongf)segment.Size) {
                Logger::error("XP3: Decompressed size does not match entry size!");
                return nullptr;
            }
        } else {
            stream.resize(entry->size);
            memcpy(stream.data(), buffer + entry->offset, entry->size);
        }

        return (const char*)stream.data();
    }
    else {
        // Encrypted entries
        Segment segment = entry->segments.at(0);
        Logger::log("%d", segment.IsCompressed);
        stream.resize(segment.Size);
        memcpy(stream.data(), buffer + segment.Offset, segment.Size);
        decrypted.resize(entry->size);
        if (segment.IsCompressed) {
            Logger::log("Decompressing Entry %s", entry->name.c_str());
            uLongf decompressed_size = (uLongf)(segment.Size);
            uLongf compressed_size = (uLongf)(segment.PackedSize);
            if (uncompress(decrypted.data(), &decompressed_size, buffer + segment.Offset, compressed_size) != Z_OK) {
                Logger::error("XP3: Failed to decompress entry!");
                return nullptr;
            }
        }
        decrypted = entry->crypt->Decrypt(entry, segment.Offset, stream, 0, stream.size());
        return (const char*)decrypted.data();

        // char* result = (char*)malloc(decrypted.size());
        // if (result) {
        //     memcpy(result, decrypted.data(), decrypted.size());
        // }
    }

    return nullptr;
}
