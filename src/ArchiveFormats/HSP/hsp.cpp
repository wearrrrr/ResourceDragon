#include "hsp.h"

long FindString(void *section_base, size_t section_size, const std::vector<unsigned char>& pattern) 
{
    if (!section_base || pattern.empty() || section_size < pattern.size())
        return -1;

    unsigned char* data = static_cast<unsigned char*>(section_base);
    size_t pattern_size = pattern.size();

    for (size_t i = 0; i <= section_size - pattern_size; ++i) {
        if (std::memcmp(&data[i], pattern.data(), pattern_size) == 0)
            return static_cast<long>(i);
    }

    return -1;
}

DPMArchive* HSPArchive::TryOpen(unsigned char *buffer, uint32_t size)
{
    bool is_pe = ExeFile::SignatureCheck(buffer);
    bool handling_exe = false;

    uint32_t dpmx_offset;
    uint32_t arc_key;

    if (is_pe) {
        handling_exe = true;
    }

    // Jank because DPMX is mentioned as a string earlier in the binary, but not referencing the data archive.
    int iter = 0;
    const unsigned char magic[] = {'D', 'P', 'M', 'X'};
    for (size_t i = 0; i < size - sizeof(magic) + 1; ++i) {
        if (std::memcmp(&buffer[i], magic, sizeof(magic)) == 0) {
            if (iter > 0) {
                dpmx_offset = i;
                arc_key = FindExeKey(buffer, dpmx_offset);

                printf("Arc Key: 0x%x\n", arc_key);
                break;
            }
            iter++;
        }
    }

    uint32_t file_count = *based_pointer<uint32_t>(buffer, dpmx_offset + 8);

    if (!IsSaneFileCount(file_count)) return nullptr;

    uint32_t index_offset = *based_pointer<uint32_t>(buffer + dpmx_offset, 0x10 + 0xC);

    printf("DPMX Offset: 0x%x\n", dpmx_offset);

    printf("File Count: 0x%x\n", file_count);

    printf("Index Offset: 0x%x\n", index_offset);


    return new DPMArchive();
}

uint32_t HSPArchive::FindExeKey(unsigned char *buffer, uint32_t dpmx_offset)
{
    return DefaultKey;

    // uint32_t base_offset = dpmx_offset - 0x10000;
    // std::string offset_str = std::to_string(base_offset) + "\0";
    // std::vector<unsigned char> offset_bytes(offset_str.begin(), offset_str.end());
    // long key_pos = -1;

    // std::vector<Pe32SectionHeader> sections = ExeFile::ParseSectionHeaders(buffer);

    // bool sec_rdata = false;
    // bool sec_data = false;

    // for (const auto& section : sections) {
    //     if (!strncmp(section.mName, ".rdata", 8)) {
    //         sec_rdata = true;
    //         uint32_t base = section.mPointerToRawData; // Offset in file
    //         uint32_t size = section.mSizeOfRawData;

    //         printf("Searching .rdata: base=0x%08x size=0x%x\n", base, size);

    //         key_pos = FindString(based_pointer(buffer, base), size, offset_bytes);
    //         if (key_pos != -1) break; // Stop if found
    //     } 
    //     else if (!strncmp(section.mName, ".data", 8)) {
    //         sec_data = true;
    //     }
    // }

    // // If not found in .rdata, search in .data
    // if (key_pos == -1 && sec_data) {
    //     for (const auto& section : sections) {
    //         if (!strncmp(section.mName, ".data", 8)) {
    //             uint32_t base = section.mPointerToRawData;
    //             uint32_t size = section.mSizeOfRawData;

    //             printf("Searching .data: base=0x%08x size=0x%x\n", base, size);

    //             key_pos = FindString(based_pointer(buffer, base), size, offset_bytes);
    //             if (key_pos != -1) break; // Stop if found
    //         }
    //     }
    // }

    // printf("rdata found: %d, data found: %d\n", sec_rdata, sec_data);
    // printf("DPMX Offset: 0x%x\n", dpmx_offset);
    // printf("Base Offset: 0x%x\n", base_offset);
    // printf("Offset String: %s\n", offset_str.c_str());
    // printf("Key Position: 0x%x\n", key_pos);

    // if (key_pos == -1)
    //     return DefaultKey; // Return a fallback if key is not found

    // // Get the actual key value at key_pos + 0x17
    // uint32_t* key_ptr = based_pointer<uint32_t>(buffer, key_pos + 0x17);
    // return *key_ptr;
}
