#pragma once

#include <filesystem>
#include <string>

#include "../zero_templates.h"

namespace fs = std::filesystem;

enum class ElfClass { 
  NONE,
  ELF32,
  ELF64
};

enum class ElfType : uint16_t {
  ET_NONE = 0,
  ET_REL = 1,
  ET_EXEC = 2,
  ET_DYN = 3,
  ET_CORE = 4,
  ET_LOOS = 0xFE00,
  ET_HIOS = 0xFEFF,
  ET_LOPROC = 0xFF00,
  ET_HIPROC = 0xFFFF
};

#define EI_NIDENT 16

struct Elf32_Header {
  unsigned char e_ident[EI_NIDENT];
  ElfType e_type;
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
  unsigned char e_ident[EI_NIDENT];
  ElfType e_type;
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



class ElfFile {
private:
  unsigned char *mFileStream;
  fs::path mFilePath;
  union {
    Elf32_Header elf32;
    Elf64_Header elf64;
  } mElfHeader;
  ElfClass mElfClass = ElfClass::NONE;
  bool mIsValid = true;

public:
  ElfFile(const fs::path &filePath) {
    mFilePath = filePath;
    auto [buffer, size] =
        read_file_to_buffer<unsigned char>(filePath.string().c_str());
    if ((ulong)size < sizeof(Elf32_Header)) {
      return;
    }
    mFileStream = buffer;

    if (mFileStream[4] == 1) {
      mElfHeader.elf32 = *(Elf32_Header *)(mFileStream);
      mElfClass = ElfClass::ELF32;
    } else if (mFileStream[4] == 2) {
      mElfHeader.elf64 = *(Elf64_Header *)(mFileStream);
      mElfClass = ElfClass::ELF64;
    } else {
      mIsValid = false;
      return;
    }
  };
  ~ElfFile() {
    free(mFileStream);
    mFileStream = nullptr;
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

  std::string GetElfType(ElfType e_type) const {
    // Determine which elf header to use for the switch case

    switch (e_type) {
      case ElfType::ET_NONE:
        return "NONE";
      case ElfType::ET_REL:
        return "REL";
      case ElfType::ET_EXEC:
        return "EXEC";
      case ElfType::ET_DYN:
        return "DYN";
      case ElfType::ET_CORE:
        return "CORE";
      case ElfType::ET_LOOS:
        return "LOOS";
      case ElfType::ET_HIOS:
        return "HIOS";
      case ElfType::ET_LOPROC:
        return "LOPROC";
      case ElfType::ET_HIPROC:
        return "HIPROC";
      default:
        return "Unknown";
    };
  };

  static bool IsValid(unsigned char *buffer);
};
