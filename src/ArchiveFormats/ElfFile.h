#pragma once

#include <string>
#include <filesystem>
#include <variant>

#include "../zero_templates.h"
#include "../util/Logger.h"

namespace fs = std::filesystem;

#define EI_NIDENT 16

struct Elf32_Header {
    unsigned char e_ident[EI_NIDENT]; // ELF identification
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};
struct Elf64_Header {
    unsigned char e_ident[EI_NIDENT]; // ELF identification
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

enum class ElfClass {
    NONE,
    ELF32,
    ELF64
};

class ElfFile {
private:
    unsigned char *mFileStream;
    size_t mFileStreamSize;
    std::string mFilePath;
    union {
        Elf32_Header elf32;
        Elf64_Header elf64;
    } mElfHeader;
    ElfClass mElfClass = ElfClass::NONE;
    bool mIsValid = true;
public:
    ElfFile(const std::string& filePath) {
        mFilePath = filePath;
        auto [buffer, size] = read_file_to_buffer<unsigned char>(filePath.c_str());
        if (size < sizeof(Elf32_Header)) {
            return;
        }
        mFileStream = buffer;
        mFileStreamSize = size;

        if (mFileStream[4] == 1) {
            mElfHeader.elf32 = *(Elf32_Header*)(mFileStream);
            mElfClass = ElfClass::ELF32;
        } else if (mFileStream[4] == 2) {
            mElfHeader.elf64 = *(Elf64_Header*)(mFileStream);
            mElfClass = ElfClass::ELF64;
        } else {
            mIsValid = false;
            return;
        }
    };
    ~ElfFile() {
        free(mFileStream);
        mFileStream = nullptr;
        mFileStreamSize = 0;
    };

    const Elf32_Header* GetElf32Header() const {
        return mElfClass == ElfClass::ELF32 ? &mElfHeader.elf32 : nullptr;
    }
    
    const Elf64_Header* GetElf64Header() const {
        return mElfClass == ElfClass::ELF64 ? &mElfHeader.elf64 : nullptr;
    }
    
    std::string GetElfClass() {
        switch (mElfClass) {
            case ElfClass::ELF32:
                return "ELF32";
            case ElfClass::ELF64:
                return "ELF64";
            default:
                return "Unknown";
        }
    }

    static bool IsValid(const fs::path &elf_path);
};