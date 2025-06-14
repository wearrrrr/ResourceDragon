#include "hsp.h"
#include <unordered_map>
#include <algorithm>

int32_t FindString(uint8_t *section_base, size_t section_size, const std::vector<uint8_t> &pattern, int step = 1)
{
    if (step <= 0) return -1;
    if (!section_base || pattern.empty() || section_size < pattern.size()) return -1;

    uint8_t *data = section_base;
    size_t pattern_size = pattern.size();
    size_t max_offset = section_size - pattern_size;

    for (size_t i = 0; i <= max_offset; i += step) {
        if (std::memcmp(&data[i], pattern.data(), pattern_size) == 0) {
            return i;
        }
    }

    return -1;
}


ArchiveBase* HSPArchive::TryOpen(uint8_t *buffer, uint64_t size, std::string file_name)
{

    ExeFile *exe = nullptr;

    uint32_t dpmx_offset = 0;
    uint32_t arc_key = 0;
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

    uint32_t file_count = Read<uint32_t>(buffer, dpmx_offset + 8);

    if (!IsSaneFileCount(file_count)) return nullptr;

    uint32_t index_offset = (dpmx_offset + 0x10) + Read<uint32_t>(exe->buffer, dpmx_offset + 0xC);
    uint32_t data_size = size - (index_offset + 32 * file_count);

    dpmx_offset += Read<uint32_t>(exe->buffer, dpmx_offset + 0x4);

    std::unordered_map<std::string, Entry> entries;
    entries.reserve(file_count);

    for (uint32_t i = 0; i < file_count; i++) {
        std::string file_name = ReadString(exe->buffer, index_offset);
        index_offset += 0x14;

        Entry entry = {
            .name = file_name,
            .key = Read<uint32_t>(exe->buffer, index_offset),
            .offset = Read<uint32_t>(exe->buffer, index_offset + 0x4) + dpmx_offset,
            .size = Read<uint32_t>(exe->buffer, index_offset + 0x8),
        };

        index_offset += 0xC;

        entries.insert({file_name, entry});
    }

    return new DPMArchive(entries, arc_key, data_size);
}

auto FindKeyFromSection(ExeFile* exe, std::string section_name, auto offset_bytes) {
    Pe32SectionHeader *section = exe->GetSectionHeader(section_name);
    uint32_t base = section->pointerToRawData;
    uint32_t size = section->sizeOfRawData;
    int32_t possible_key_pos = FindString(based_pointer<uint8_t>(exe->buffer, base), size, offset_bytes);

    return std::make_pair(possible_key_pos, base);
}

uint32_t HSPArchive::FindExeKey(ExeFile* exe, uint32_t dpmx_offset)
{
    std::string offset_str = std::to_string(dpmx_offset - 0x10000) + "\0";
    std::vector<uint8_t> offset_bytes(offset_str.begin(), offset_str.end());
    int32_t key_pos = -1;
    uint32_t found_section_offset = 0x0;

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

    return Read<uint32_t>(exe->buffer, (found_section_offset + key_pos) + 0x17);
}

bool HSPArchive::CanHandleFile(uint8_t *buffer, uint64_t size, const std::string &ext) const
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

const char *DPMArchive::OpenStream(const Entry *entry, uint8_t *buffer)
{
    uint8_t *data = buffer + entry->offset;

    if (entry->key) {
        return (const char*)DecryptEntry(data, entry->size, entry->key);
    }

    return (const char*)(data);
}
