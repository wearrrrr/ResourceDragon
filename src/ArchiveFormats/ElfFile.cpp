#include "ElfFile.h"

bool ElfFile::IsValid(const fs::path &elf_path)
{
    // Check that the file is valid and that it matches the ELF magic number
    if (!fs::exists(elf_path)) {
        return false;
    }
    ElfFile elf(elf_path.string());

    // Check if the first 4 bytes match the ELF magic number
    uint32_t magic = *based_pointer<uint32_t>(elf.mFileStream, 0);
    // ELF magic number is 0x7F followed by 'E', 'L', 'F'
    if (magic != PackUInt(0x7F, 'E', 'L', 'F')) {
        free(elf.mFileStream);
        elf.mFileStream = nullptr;
        elf.mFileStreamSize = 0;
        return false;
    }
    // If the magic number doesn't match, it's not a valid ELF file
    free(elf.mFileStream);
    elf.mFileStream = nullptr;
    elf.mFileStreamSize = 0;

    return true;
}