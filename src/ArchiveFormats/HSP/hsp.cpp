#include "hsp.h"

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
            sec_rdata = true;
            break;
        } else if (!strncmp(section.mName, ".data", 8)) {
            sec_data = true;
            break;
        }
    }

    if (sec_rdata) {
        // key_pos = 
    }

    printf("rdata: %d data: %d\n", sec_rdata, sec_data);

    printf("0x%x\n", base_offset);
    printf("%s\n", offset_str.c_str());

    return 0;
}
