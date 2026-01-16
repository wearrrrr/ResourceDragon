#pragma once
#include <string>

#include <SDK/util/Logger.hpp>
#include <util/int.h>
#include "zero_templates.h"

/**
  ELF spec information obtained from: https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.eheader.html
**/

enum class ElfABI : u8 {
  NONE = 0,
  HPUX = 1,
  NETBSD = 2,
  GNU = 3,
  SOLARIS = 6,
  AIX = 7,
  IRIX = 8,
  FREEBSD = 9,
  TRU64 = 10,
  MODESTO = 11,
  OPENBSD = 12,
  OPENVMS = 13,
};

enum class ElfClass : u8 {
  NONE = 0,
  ELF32 = 1,
  ELF64 = 2,
};

#define EI_NIDENT 16

enum class ElfType : u16 {
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

struct ElfIdent {
  u8 magic[4];
  ElfClass class_type;
  u8 data_encoding;
  u8 version;
  ElfABI os_abi;
  u8 abi_version;
  u8 pad[7];
};
static_assert(sizeof(ElfIdent) == EI_NIDENT);

struct Elf32_Header {
  union {
    u8 e_ident_raw[EI_NIDENT];
    ElfIdent e_ident;
  };
  ElfType e_type;
  u16 e_machine;
  u32 e_version;
  u32 e_entry;
  u32 e_phoff;
  u32 e_shoff;
  u32 e_flags;
  u16 e_ehsize;
  u16 e_phentsize;
  u16 e_phnum;
  u16 e_shentsize;
  u16 e_shnum;
  u16 e_shstrndx;
};
struct Elf64_Header {
  union {
    u8 e_ident_raw[EI_NIDENT];
    ElfIdent e_ident;
  };
  ElfType e_type;
  u16 e_machine;
  u32 e_version;
  u64 e_entry;
  u64 e_phoff;
  u64 e_shoff;
  u32 e_flags;
  u16 e_ehsize;
  u16 e_phentsize;
  u16 e_phnum;
  u16 e_shentsize;
  u16 e_shnum;
  u16 e_shstrndx;
};



class ElfFile {
private:
  union {
    Elf32_Header elf32;
    Elf64_Header elf64;
  } mElfHeader;
  ElfClass mElfClass = ElfClass::NONE;
  bool mIsValid = true;

public:
  ElfFile(const u8 *buffer, u64 size) {
    if (size < 52L) {
      Logger::error("File is smaller than minimum possible elf size! This is not a valid ELF file.");
      mIsValid = false;
      return;
    }

    ElfClass elfClass = (ElfClass)(buffer[4]);

    if (elfClass == ElfClass::ELF32) {
      mElfHeader.elf32 = *(Elf32_Header *)(buffer);
      mElfClass = ElfClass::ELF32;
    } else if (elfClass == ElfClass::ELF64) {
      mElfHeader.elf64 = *(Elf64_Header *)(buffer);
      mElfClass = ElfClass::ELF64;
    } else {
      mIsValid = false;
      return;
    }
  };

  const Elf32_Header* GetElf32Header() const {
    return mElfClass == ElfClass::ELF32 ? &mElfHeader.elf32 : nullptr;
  }

  const Elf64_Header* GetElf64Header() const {
    return mElfClass == ElfClass::ELF64 ? &mElfHeader.elf64 : nullptr;
  }

  std::string GetElfOSABI(ElfABI abi) const {
    switch (abi) {
      case ElfABI::NONE: return "System V";
      case ElfABI::HPUX: return "Hewlett-Packard HP-UX";
      case ElfABI::NETBSD: return "NetBSD";
      case ElfABI::GNU: return "GNU/Linux (Deprecated)";
      case ElfABI::SOLARIS: return "Solaris";
      case ElfABI::AIX: return "AIX";
      case ElfABI::IRIX: return "IRIX";
      case ElfABI::FREEBSD: return "FreeBSD";
      case ElfABI::TRU64: return "Compaq TRU64 UNIX";
      case ElfABI::MODESTO: return "Novell - Modesto";
      case ElfABI::OPENBSD: return "OpenBSD";
      case ElfABI::OPENVMS: return "OpenVMS";
      default: return "Unknown";
    }
  };

  std::string GetElfClass() const {
    switch (mElfClass) {
    case ElfClass::ELF32:
      return "ELF32";
    case ElfClass::ELF64:
      return "ELF64";
    default:
      return "Unknown";
    }
  };

  std::string GetElfType(ElfType e_type) const {
    switch (e_type) {
      case ElfType::ET_NONE: return "NONE";
      case ElfType::ET_REL: return "REL";
      case ElfType::ET_EXEC: return "EXEC";
      case ElfType::ET_DYN: return "DYN";
      case ElfType::ET_CORE: return "CORE";
      case ElfType::ET_LOOS: return "LOOS";
      case ElfType::ET_HIOS: return "HIOS";
      case ElfType::ET_LOPROC: return "LOPROC";
      case ElfType::ET_HIPROC: return "HIPROC";
      default: return "Unknown";
    };
  };



  static bool IsValid(u8 *buffer)
  {
      u32 magic = *based_pointer<u32>(buffer, 0);
      return magic == PackUInt(0x7F, 'E', 'L', 'F');
  }
};
