#include "hsp.h"

uint32_t FindString(unsigned char *section_base, size_t section_size, const std::vector<unsigned char> &pattern, int step = 1)
{
    if (step <= 0) return -1;
    if (!section_base || pattern.empty() || section_size < pattern.size()) return -1;

    unsigned char* data = section_base;
    size_t pattern_size = pattern.size();
    size_t max_offset = section_size - pattern_size;

    for (size_t i = 0; i <= max_offset; i += step) {
        if (std::memcmp(&data[i], pattern.data(), pattern_size) == 0) {
            return i;
        }
    }

    return -1;
}

std::string ReadString(unsigned char *buffer, uint32_t offset, size_t read_amount) {
    std::string constructed = "";
    constructed.append(based_pointer<char>(buffer, offset));

    return constructed;
}


DPMArchive* HSPArchive::TryOpen(unsigned char *buffer, uint32_t size)
{

    ExeFile *exe = nullptr;

    uint32_t dpmx_offset;
    uint32_t arc_key;
    bool is_pe = false;

    if (ExeFile::SignatureCheck(buffer)) {
        is_pe = true;
    }

    // Jank because DPMX is mentioned as a string earlier in the binary, but not referencing the data archive.
    // This needs more testing to see if I can get away with adding 0x90 to the sig, since that seems to be something different between them.
    int iter = 0;
    if (is_pe) {
        exe = ConvertToExeFile(buffer);
        for (size_t i = 0; i < size - sizeof(sig) + 1; ++i) {
            if (std::memcmp(&exe->raw_contents[i], &sig, sizeof(sig)) == 0) {
                if (iter > 0) {
                    dpmx_offset = i;
                    arc_key = FindExeKey(exe, dpmx_offset);

                    printf("Arc Key: 0x%x\n", arc_key);
                    break;
                }
                iter++;
            }
        }
        if (iter == 0) {
            printf("Could not find 'DPMX' in the binary! Are you sure this game has a valid archive?");
        }
    } else {
        throw std::invalid_argument("Extracting from non-exe targets is currently not supported!");
    }

    uint32_t file_count = *based_pointer<uint32_t>(buffer, dpmx_offset + 8);

    if (!IsSaneFileCount(file_count)) return nullptr;
    printf("File Count: 0x%x\n", file_count);

    uint32_t index_offset = (dpmx_offset + 0x10) + ReadUint32(exe->raw_contents, dpmx_offset + 0xC);
    uint32_t data_size = size - (index_offset + 32 * file_count);

    printf("index_offset: 0x%x\n", index_offset);
    printf("data size: 0x%x\n", data_size);
    dpmx_offset += *based_pointer<uint32_t>(exe->raw_contents, dpmx_offset + 0x4);
    
    std::vector<DPMEntry> entries;
    entries.reserve(file_count);

    for (int i = 0; i < file_count; i++) {
        std::string file_name =  ReadString(exe->raw_contents, index_offset, 0x10);
        index_offset += 0x14;

        DPMEntry entry;
        entry.name = file_name;
        entry.key = *based_pointer<uint32_t>(exe->raw_contents, index_offset);
        entry.offset = *based_pointer<uint32_t>(exe->raw_contents, index_offset + 0x4) + dpmx_offset;
        entry.size = *based_pointer<uint32_t>(exe->raw_contents, index_offset + 0x8);

        index_offset += 0xC;

        entries.push_back(entry);
    }
    DPMEntry first_entry = entries.at(1);

    return new DPMArchive(entries, arc_key, data_size);
}

auto FindKeyFromSection(ExeFile* exe, std::string section_name, std::vector<unsigned char> offset_bytes) {
    Pe32SectionHeader *section = exe->GetSectionHeader(section_name);
    uint32_t base = section->mPointerToRawData;
    uint32_t size = section->mSizeOfRawData;
    printf("Searching %s: base=0x%08x size=0x%x\n", section->mName, base, size);

    uint32_t possible_key_pos = FindString(based_pointer<unsigned char>(exe->raw_contents, base), size, offset_bytes);

    return std::make_pair(possible_key_pos, base);
}

uint32_t HSPArchive::FindExeKey(ExeFile* exe, uint32_t dpmx_offset)
{
    std::string offset_str = std::to_string(dpmx_offset - 0x10000) + "\0";
    std::vector<unsigned char> offset_bytes(offset_str.begin(), offset_str.end());
    uint32_t key_pos = -1;
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

    printf("DPMX Offset: 0x%x\n", dpmx_offset);
    printf("Key Position: 0x%x\n", found_section_offset + key_pos);

    if (key_pos == -1) {
        printf("Failed to find key! Returning default key...\n");
        return DefaultKey;
    };
    // ptr to where the exe key is held
    return *based_pointer<uint32_t>(exe->raw_contents, (found_section_offset + key_pos) + 0x17);
}
