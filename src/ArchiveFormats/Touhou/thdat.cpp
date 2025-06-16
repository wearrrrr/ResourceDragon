#include "thdat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unordered_map>

// This code comes from a slightly modified version provided to me by VieRo (https://github.com/PieVieRo)
// All credit to them! This format is a bit of a mess..

static void getNextByte(BitReader *reader) {
    if (reader->position < reader->size) {
        reader->current_byte = reader->buffer[reader->position++];
        reader->bitmask = 0x80;
    } else {
        reader->current_byte = 0;
        reader->bitmask = 0;
    }
}

static void byteSeek(BitReader *reader, unsigned offset) {
    if (offset < reader->size) {
        reader->position = offset;
        getNextByte(reader);
    }
}

static u8 testBit(BitReader *reader) {
    return (reader->current_byte & reader->bitmask) != 0;
}

unsigned readBits(BitReader *reader, unsigned bits) {
    unsigned result = 0;
    while (bits--) {
        if (!reader->bitmask) getNextByte(reader);
        result <<= 1;
        result |= testBit(reader);
        reader->bitmask >>= 1;
    }
    return result;
}

unsigned readInt(BitReader *reader) {
    unsigned variant = readBits(reader, 2);
    return readBits(reader, 8 * (variant + 1));
}

char readString(BitReader *reader, char *out, unsigned max) {
    for (unsigned idx = 0; idx < max; ++idx) {
        out[idx] = readBits(reader, 8);
        if (out[idx] == 0) return 0;
    }
    return -1;
}

unsigned readMagic(BitReader *reader) {
    return readBits(reader, 8) |
           (readBits(reader, 8) << 8) |
           (readBits(reader, 8) << 16) |
           (readBits(reader, 8) << 24);
}

char parseDatHeader(DatFile *dat) {
    if (readMagic(&dat->reader) != PackUInt32('P', 'B', 'G', '3'))
        return -1;

    dat->entries_count = readInt(&dat->reader);
    dat->file_table_offset = readInt(&dat->reader);
    byteSeek(&dat->reader, dat->file_table_offset);

    dat->entries = (DatEntry*)malloc(sizeof(DatEntry) * dat->entries_count);
    if (!dat->entries) return -1;

    for (unsigned idx = 0; idx < dat->entries_count; ++idx) {
        dat->entries[idx].unk[0] = readInt(&dat->reader);
        dat->entries[idx].unk[1] = readInt(&dat->reader);
        dat->entries[idx].checksum = readInt(&dat->reader);
        dat->entries[idx].data_offset = readInt(&dat->reader);
        dat->entries[idx].size = readInt(&dat->reader);
        readString(&dat->reader, dat->entries[idx].name, ENTRY_NAME_LENGTH);
    }

    return 0;
}

char openDatFromBuffer(DatFile *dat, const u8 *buffer, size_t size) {
    dat->reader.buffer = buffer;
    dat->reader.size = size;
    dat->reader.position = 0;
    dat->reader.bitmask = 0;
    dat->reader.current_byte = 0;

    return parseDatHeader(dat);
}

int findPbg3Entry(DatFile *dat, const char *entry) {
    for (unsigned idx = 0; idx < dat->entries_count; ++idx) {
        if (strcmp(entry, dat->entries[idx].name) == 0)
            return idx;
    }
    return -1;
}

#define LZSS_DICTIONARY_SIZE 0x2000
#define LZSS_OFFSET 13
#define LZSS_LENGTH 4
#define LZSS_MIN_MATCH 3

void *decompressEntry(DatFile *dat, unsigned idx) {
    char dictionary[LZSS_DICTIONARY_SIZE] = {0};
    unsigned short dict_idx = 0;

    const unsigned data_offset = dat->entries[idx].data_offset;
    // const unsigned next_data_start = (idx == dat->entries_count - 1)
    //     ? dat->file_table_offset
    //     : dat->entries[idx + 1].data_offset;

    // const unsigned compressed_size = next_data_start - data_offset;
    const unsigned uncompressed_size = dat->entries[idx].size;

    byteSeek(&dat->reader, data_offset);

    u8 *out = (u8*)malloc(uncompressed_size);
    if (!out) return NULL;

    unsigned out_idx = 0;
    while (out_idx < uncompressed_size) {
        u8 is_raw = readBits(&dat->reader, 1);
        if (is_raw) {
            u8 b = readBits(&dat->reader, 8);
            out[out_idx++] = b;
            dictionary[dict_idx++] = b;
            dict_idx %= LZSS_DICTIONARY_SIZE;
        } else {
            unsigned short offset = readBits(&dat->reader, LZSS_OFFSET) - 1;
            u8 length = readBits(&dat->reader, LZSS_LENGTH) + LZSS_MIN_MATCH;
            for (int j = offset; j < offset + length && out_idx < uncompressed_size; ++j) {
                u8 b = dictionary[j % LZSS_DICTIONARY_SIZE];
                out[out_idx++] = b;
                dictionary[dict_idx++] = b;
                dict_idx %= LZSS_DICTIONARY_SIZE;
            }
        }
    }

    getNextByte(&dat->reader); // ensure next byte is ready for next read
    return out;
}


ArchiveBase *THDAT::TryOpenTH06(u8 *buffer, u64 size, std::string file_name) {
    DatFile dat = {};
    std::unordered_map<std::string, DatEntry> entries;
    openDatFromBuffer(&dat, buffer, size);
    for (unsigned idx = 0; idx < dat.entries_count; ++idx) {
        entries.emplace(std::string(dat.entries[idx].name), dat.entries[idx]);
    }

    return new THDATArchive(dat, entries);
}

ArchiveBase *THDAT::TryOpen(u8 *buffer, u64 size, std::string file_name) {
    return TryOpenTH06(buffer, size, file_name);
}
