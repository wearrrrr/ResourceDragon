#include "xp3.h"
#include "Entry.h"
#include "XP3/Crypt/Crypt.h"
#include "../../util/Text.h"
#include <cstring>

static XP3Crypt *ALG_DEFAULT = new NoCrypt();

ArchiveBase *XP3Format::TryOpen(u8 *buffer, u64 size, std::string file_name) {
    int64_t base_offset = 0;

    if (!CanHandleFile(buffer, size, "")) {
        Logger::error("XP3: Invalid file signature!");
        return nullptr;
    }

    u64 dir_offset = base_offset + Read<u64>(buffer, base_offset + 0x0B);

    if (dir_offset < 0x13 || dir_offset >= size) return nullptr;

    if (Read<u32>(buffer, dir_offset) == 0x80) {
        dir_offset = base_offset + Read<int64_t>(buffer, dir_offset + 0x9);
        if (dir_offset < 0x13) return nullptr;
    }

    int8_t header_type = Read<int8_t>(buffer, dir_offset);
    if (header_type != XP3_HEADER_UNPACKED && header_type != XP3_HEADER_PACKED) {
        Logger::error("XP3: Header type is invalid!");
        return nullptr;
    }
    Logger::log("XP3 Header type: %s", header_type == XP3_HEADER_UNPACKED ? "Unpacked" : "Packed");

    std::vector<u8> header_stream;
    if (header_type == XP3_HEADER_UNPACKED) {
        int64_t header_size = Read<int64_t>(buffer, dir_offset + 0x1);
        if ((u64)header_size > UINT64_MAX) {
            Logger::error("XP3: Header size is invalid!");
            return nullptr;
        }
        header_stream.resize(header_size);
        memcpy(header_stream.data(), buffer + dir_offset + 0x9, (u32)header_size);
    } else {
#ifdef NO_ZLIB
        return nullptr;
#else
        int64_t packed_size = Read<int64_t>(buffer, dir_offset + 0x1);
        if ((u64)packed_size > UINT64_MAX) {
            Logger::error("XP3: Packed size is invalid!");
            return nullptr;
        }
        int64_t header_size = Read<int64_t>(buffer, dir_offset + 0x9);
        const u8* compressed_data = buffer + dir_offset + 0x11;
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
#endif
    }

    EntryMap dir;
    dir_offset = 0;

    BinaryReader header(header_stream);

    while (header.peek().has_value()) {
        u32 entry_signature = header.read<u32>();
        int64_t entry_size = header.read<int64_t>();
        if (entry_size < 0) {
            Logger::error("XP3: Entry size is invalid!");
            return nullptr;
        }
        dir_offset += 12 + entry_size;

        if (entry_signature == PackUInt32('F', 'i', 'l', 'e')) {
            Entry entry = {};
            while (entry_size > 0) {
                u32 section = header.read<u32>();
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
                        entry.isEncrypted = header.read<u32>() != 0;
                        u64 file_size = header.read<u64>();
                        u64 packed_size = header.read<int64_t>();
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
                        i32 segment_count = section_size / 0x1C;
                        if (segment_count > 0) {
                            for (int i = 0; i < segment_count; ++i) {
                                bool compressed = header.read<i32>() != 0;
                                u64 segment_offset = base_offset + header.read<u64>();
                                int64_t segment_size = header.read<int64_t>();
                                u64 segment_packed_size = header.read<u64>();
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
                            entry.hash = header.read<u32>();
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
            header.read<u32>();
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

std::vector<u8> DecompressLz4(const Entry *entry, std::vector<u8> buffer) {
    Logger::error("DecompressLZ4 called! We don't support this yet...");
    return {};
};
std::vector<u8> DecompressMdf(const Entry *entry, std::vector<u8> buffer) {
    Logger::error("DecompressMdf called! We don't support this yet...");
    return {};
};

std::vector<u8> DecryptScript(int enc_type, const std::vector<u8>& input, u32 unpacked_size) {
    size_t input_size = input.size();

    if (enc_type == 2) {
        // Check we have at least 16 bytes for two Int64 reads
        if (input_size < 16)
            throw std::runtime_error("Input too short for enc_type 2");
        std::vector<u8> compressed_data(input.begin() + 16, input.end());
        return {};
    }

    // For enc_type 0 or 1, prepare output vector with capacity +2 bytes for BOM
    std::vector<u8> output;
    output.reserve(unpacked_size);

    output.push_back(0xFF);
    output.push_back(0xFE);

    for (size_t pos = 0; pos + 1 < input_size; pos += 2) {
        u16 c = input[pos] | (input[pos + 1] << 8);

        if (enc_type == 1) {
            c = ((c & 0xAAAA) >> 1) | ((c & 0x5555) << 1);
        } else if (c < 0x20) {
            continue;
        } else {
            c = c ^ (((c & 0xFE) << 8) ^ 1);
        }
        output.push_back((u8)c & 0xFF);
        output.push_back((u8)c >> 8);

        if (output.size() >= unpacked_size)
            break;
        // Janky fix but whatever
        output[output.size() - 2] = '\0';
    }
    return output;
}

std::vector<u8> EntryReadFilter(const Entry *entry, const std::vector<u8>& buffer) {
    if (entry->size <= 5 /* || entry->Type == "audio" */)
        return buffer;

    if (buffer.size() < 5)
        return buffer;

    u32 signature = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

    if (signature == 0x184D2204) {
        // LZ4 magic
        return DecompressLz4(entry, buffer);
    }

    u32 mdfSig = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16);
    if (mdfSig == 0x00666D64) { // 'mdf' little-endian
        return DecompressMdf(entry, buffer);
    }

    if ((signature & 0xFF00FFFFu) == 0xFF00FEFEu && buffer[2] < 3 && buffer[4] == 0xFE) {
        std::vector<u8> script_data(buffer.begin() + 5, buffer.end());
        return DecryptScript(buffer[2], script_data, entry->size);
    }

    return buffer;
}

u8* XP3Archive::OpenStream(const Entry *entry, u8 *buffer) {
    Segment segment = entry->segments.at(0);
    std::vector<u8> stream(entry->size);

    if (entry->segments.size() == 1 && !entry->isEncrypted) {
        if (segment.IsCompressed) {
#ifdef NO_ZLIB
            return {};
#else
            uLongf decompressed_size = entry->size;
            uLongf compressed_size = entry->packedSize;
            if (uncompress(stream.data(), &decompressed_size, buffer + entry->offset, compressed_size) != Z_OK) {
                Logger::error("XP3: Failed to decompress entry!");
                return {};
            }
#endif
        } else {
            memcpy(stream.data(), buffer + entry->offset, entry->size);
        }

        auto filter = EntryReadFilter(entry, stream);
        u8 *buf = (u8*)malloc(entry->size);
        memcpy(buf, filter.data(), entry->size);

        return buf;
    } else {
        memcpy(stream.data(), buffer + entry->offset, entry->size);
        std::vector<u8> decrypted = entry->crypt->Decrypt(entry, 0, stream, 0, entry->size);
        auto filter = EntryReadFilter(entry, decrypted);
        u8 *buf = (u8*)malloc(entry->size);
        memcpy(buf, filter.data(), entry->size);

        return buf;
    }
}
