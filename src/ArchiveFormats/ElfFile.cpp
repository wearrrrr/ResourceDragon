#include "ElfFile.h"

bool ElfFile::IsValid(unsigned char *buffer)
{
    // Check if the first 4 bytes match the ELF magic number
    uint32_t magic = *based_pointer<uint32_t>(buffer, 0);
    return magic == PackUInt(0x7F, 'E', 'L', 'F');
}