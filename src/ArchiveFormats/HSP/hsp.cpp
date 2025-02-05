#include "hsp.h"

long FindString(void* section_base, size_t section_size, const std::vector<unsigned char>& pattern) 
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

uint32_t HSPArchive::TryOpen(unsigned char *buffer, uint32_t size)
{
    bool is_pe = ExeFile::SignatureCheck(buffer);
    bool handling_exe = false;

    uint32_t dpmx_offset;
    uint32_t arc_key;

    if (is_pe) {
        handling_exe = true;
    }

    const unsigned char magic[] = {'D', 'P', 'M', 'X'};
    for (size_t i = 0; i < size - sizeof(magic) + 1; ++i) {
        if (std::memcmp(&buffer[i], magic, sizeof(magic)) == 0) {
            dpmx_offset = i;
            arc_key = FindExeKey(buffer, dpmx_offset);
            break;
        }
    }


    return 0;
}

uint32_t HSPArchive::FindExeKey(unsigned char *buffer, uint32_t dpmx_offset)
{
    uint32_t base_offset = dpmx_offset - 0x10000;
    std::string offset_str = std::to_string(base_offset) + "\0";
    std::vector<unsigned char> offset_bytes(offset_str.begin(), offset_str.end());
    long key_pos = -1;

    std::vector<Pe32SectionHeader> sections = ExeFile::ParseSectionHeaders(buffer);

    bool sec_rdata = false;
    bool sec_data = false;


    for (auto section : sections) {
        if (!strncmp(section.mName, ".rdata", 8)) {
                key_pos = *based_pointer<long>(buffer + section.mPointerToRawData, 0);
            break;
        } else if (!strncmp(section.mName, ".data", 8)) {
            sec_data = true;
            break;
        }
    }

    printf("rdata: %d data: %d\n", sec_rdata, sec_data);

    printf("0x%x\n", base_offset);
    printf("%s\n", offset_str.c_str());
    printf("key pos: 0x%x\n", key_pos);



    return 0;
}
