#include "hsp.h"

long FindString(void* section_base, size_t section_size, const std::vector<unsigned char>& pattern, int step = 1)
{
    if (step <= 0) return -1; // Step must be positive
    if (!section_base || pattern.empty() || section_size < pattern.size()) return -1;

    unsigned char* data = (unsigned char*)(section_base);
    size_t pattern_size = pattern.size();
    size_t max_offset = section_size - pattern_size;

    for (size_t i = 0; i <= max_offset; i += step) {
        if (std::memcmp(&data[i], pattern.data(), pattern_size) == 0) {
            return (long)(i); // Found match, return offset
        }
    }

    return -1; // Not found
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

    uint32_t c_offset = *based_pointer<uint32_t>(buffer, dpmx_offset + 0xC);

    printf("File Count: 0x%x\n", file_count);

    return new DPMArchive();
}

uint32_t HSPArchive::FindExeKey(unsigned char *buffer, uint32_t dpmx_offset)
{
    uint32_t base_offset = dpmx_offset - 0x10000;
    std::string offset_str = std::to_string(base_offset) + "\0";
    std::vector<unsigned char> offset_bytes(offset_str.begin(), offset_str.end());
    uint32_t key_pos = -1;

    uint32_t found_section_offset = 0x0;

    if (ExeFile::ContainsSection(buffer, ".rdata")) {
        Pe32SectionHeader section = *ExeFile::GetSectionHeader(buffer, ".rdata");
        uint32_t base = section.mPointerToRawData;
        uint32_t size = section.mSizeOfRawData;

        printf("Searching %s: base=0x%08x size=0x%x\n", section.mName, base, size);

        key_pos = FindString(based_pointer(buffer, base), size, offset_bytes);
        if (key_pos == -1) printf("Couldn't find in %s!\n", section.mName);
    }
    if (key_pos == -1 && ExeFile::ContainsSection(buffer, ".data")) {
        Pe32SectionHeader section = *ExeFile::GetSectionHeader(buffer, ".data");
        uint32_t base = section.mPointerToRawData;
        uint32_t size = section.mSizeOfRawData;
        found_section_offset = base;
        printf("Searching %s: base=0x%08x size=0x%x\n", section.mName, base, size);
        key_pos = FindString(based_pointer(buffer, base), size, offset_bytes);
    }
    printf("DPMX Offset: 0x%x\n", dpmx_offset);
    printf("Base Offset: 0x%x\n", base_offset);
    printf("Offset String: %s\n", offset_str.c_str());
    printf("Key Position: 0x%x\n", found_section_offset + key_pos);

    if (key_pos == -1) return DefaultKey;

    // Get the actual key value at key_pos + 0x17
    uint32_t* key_ptr = based_pointer<uint32_t>(buffer, found_section_offset + key_pos + 0x17);
    return std::byteswap(*key_ptr);
}
