#include "hsp.h"

long FindString(unsigned char* section_base, size_t section_size, const std::vector<unsigned char>& pattern, int step = 1)
{
    if (step <= 0) return -1; // Step must be positive
    if (!section_base || pattern.empty() || section_size < pattern.size()) return -1;

    unsigned char* data = section_base;
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
            if (std::memcmp(&buffer[i], &sig, sizeof(sig)) == 0) {
                if (iter > 0) {
                    dpmx_offset = i;
                    arc_key = FindExeKey(exe, dpmx_offset);

                    printf("Arc Key: 0x%x\n", arc_key);
                    break;
                }
                iter++;
            }
        }
    } else {
        throw std::invalid_argument("Extracting from non-exe targets is currently not supported!");
    }

    uint32_t file_count = *based_pointer<uint32_t>(buffer, dpmx_offset + 8);

    if (!IsSaneFileCount(file_count)) return nullptr;
    printf("File Count: 0x%x\n", file_count);

    return new DPMArchive();
}

uint32_t HSPArchive::FindExeKey(ExeFile* exe, uint32_t dpmx_offset)
{
    std::string offset_str = std::to_string(dpmx_offset - 0x10000) + "\0";
    std::vector<unsigned char> offset_bytes(offset_str.begin(), offset_str.end());
    
    uint32_t key_pos = -1;
    uint32_t found_section_offset = 0x0;

    if (exe->ContainsSection(".rdata")) {
        Pe32SectionHeader *section = exe->GetSectionHeader(".rdata");
        uint32_t base = section->mPointerToRawData;
        uint32_t size = section->mSizeOfRawData;

        printf("Searching %s: base=0x%08x size=0x%x\n", section->mName, base, size);

        key_pos = FindString(based_pointer<unsigned char>(exe->raw_contents, base), size, offset_bytes);
    }
    if (key_pos == -1 && exe->ContainsSection(".data")) {
        Pe32SectionHeader *section = exe->GetSectionHeader(".data");
        uint32_t base = section->mPointerToRawData;
        uint32_t size = section->mSizeOfRawData;
        found_section_offset = base;
        printf("Searching %s: base=0x%08x size=0x%x\n", section->mName, base, size);
        key_pos = FindString(based_pointer<unsigned char>(exe->raw_contents, base), size, offset_bytes);
    }
    printf("DPMX Offset: 0x%x\n", dpmx_offset);
    printf("Key Position: 0x%x\n", found_section_offset + key_pos);

    if (key_pos == -1) return DefaultKey;

    // Get the actual key value at key_pos + 0x17
    uint32_t* key_ptr = based_pointer<uint32_t>(exe->raw_contents, found_section_offset + key_pos + 0x17);
    return *key_ptr;
}
