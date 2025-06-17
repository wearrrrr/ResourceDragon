#include "hsp.h"
#include <unordered_map>
#include <algorithm>

i32 FindString(u8 *section_base, size_t section_size, const std::vector<u8> &pattern, int step = 1)
{
    if (step <= 0) return -1;
    if (!section_base || pattern.empty() || section_size < pattern.size()) return -1;

    u8 *data = section_base;
    size_t pattern_size = pattern.size();
    size_t max_offset = section_size - pattern_size;

    for (size_t i = 0; i <= max_offset; i += step) {
        if (std::memcmp(&data[i], pattern.data(), pattern_size) == 0) {
            return i;
        }
    }

    return -1;
}


ArchiveBase* HSPArchive::TryOpen(u8 *buffer, u64 size, std::string file_name)
{

    ExeFile *exe = nullptr;

    u32 dpmx_offset = 0;
    u32 arc_key = 0;
    bool is_pe = ExeFile::SigCheck(buffer);

    // Jank because DPMX is mentioned as a string earlier in the binary, but not referencing the data archive.
    // This needs more testing to see if I can get away with adding 0x90 to the sig, since that seems to be something different between them.
    int iter = 0;
    if (!is_pe) {
        Logger::warn("Extracting from non-exe targets is currently not supported!");
        return nullptr;
    }

    exe = ConvertToExeFile(buffer);
    for (size_t i = 0; i < size - sizeof(sig) + 1; ++i) {
        if (std::memcmp(&exe->buffer[i], &sig, sizeof(sig)) == 0) {
            if (iter > 0) {
                dpmx_offset = i;
                arc_key = FindExeKey(exe, dpmx_offset);
                break;
            }
            iter++;
        }
    }
    if (iter == 0) {
        Logger::error("Could not find 'DPMX' in the binary! Are you sure this game has a valid archive?");
    }

    u32 file_count = Read<u32>(buffer, dpmx_offset + 8);

    if (!IsSaneFileCount(file_count)) return nullptr;

    u32 index_offset = (dpmx_offset + 0x10) + Read<u32>(exe->buffer, dpmx_offset + 0xC);
    u32 data_size = size - (index_offset + 32 * file_count);

    dpmx_offset += Read<u32>(exe->buffer, dpmx_offset + 0x4);

    std::unordered_map<std::string, Entry> entries;
    entries.reserve(file_count);

    for (u32 i = 0; i < file_count; i++) {
        std::string file_name = ReadString(exe->buffer, index_offset);
        index_offset += 0x14;

        Entry entry = {
            .name = file_name,
            .key = Read<u32>(exe->buffer, index_offset),
            .offset = Read<u32>(exe->buffer, index_offset + 0x4) + dpmx_offset,
            .size = Read<u32>(exe->buffer, index_offset + 0x8),
        };

        index_offset += 0xC;

        entries.insert({file_name, entry});
    }

    return new DPMArchive(entries, arc_key, data_size);
}

auto FindKeyFromSection(ExeFile* exe, std::string section_name, auto offset_bytes) {
    Pe32SectionHeader *section = exe->GetSectionHeader(section_name);
    u32 base = section->pointerToRawData;
    u32 size = section->sizeOfRawData;
    i32 possible_key_pos = FindString(based_pointer<u8>(exe->buffer, base), size, offset_bytes);

    return std::make_pair(possible_key_pos, base);
}

u32 HSPArchive::FindExeKey(ExeFile* exe, u32 dpmx_offset)
{
    std::string offset_str = std::to_string(dpmx_offset - 0x10000) + "\0";
    std::vector<u8> offset_bytes(offset_str.begin(), offset_str.end());
    i32 key_pos = -1;
    u32 found_section_offset = 0x0;

    if (exe->ContainsSection(".rdata")) {
        auto [search, base] = FindKeyFromSection(exe, ".rdata", offset_bytes);
        if (search != -1) {
            key_pos = search;
            found_section_offset = base;
        }
    }
    if (key_pos == -1 && exe->ContainsSection(".data")) {
        auto [search, base] = FindKeyFromSection(exe, ".data", offset_bytes);
        if (search != -1) {
            key_pos = search;
            found_section_offset = base;
        }
    }

    if (key_pos == -1) {
        printf("Failed to find key! Returning default key...\n");
        return DefaultKey;
    };

    return Read<u32>(exe->buffer, (found_section_offset + key_pos) + 0x17);
}

bool HSPArchive::CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const
{
    if (std::find(extensions.begin(), extensions.end(), ext) == extensions.end()) {
        return false;
    }

    int iter = 0;
    for (size_t i = 0; i < size - sizeof(sig) + 1; ++i) {
        if (std::memcmp(&buffer[i], &sig, sizeof(sig)) == 0) {
            if (iter > 0) {
                return true;
            }
            iter++;
        }
    }

    return false;
}

const char *DPMArchive::OpenStream(const Entry *entry, u8 *buffer)
{
    u8 *data = buffer + entry->offset;

    if (entry->key) {
        return (const char*)DecryptEntry(data, entry->size, entry->key);
    }

    return (const char*)(data);
}
