#pragma once

#include <string>
#include <map>
#include "util/int.h"

struct PeHeader {
	u32 mMagic; // PE\0\0
	u16 mMachine;
	u16 mNumberOfSections;
	u32 mTimeDateStamp;
	u32 mPointerToSymbolTable;
	u32 mNumberOfSymbols;
	u16 mSizeOfOptionalHeader;
	u16 mCharacteristics;
};

struct Pe32OptionalHeader {
	u16 mMagic; // 0x010b - PE32, 0x020b - PE32+ (64 bit)
	u8 mMajorLinkerVersion;
	u8 mMinorLinkerVersion;
	u32 mSizeOfCode;
	u32 mSizeOfInitializedData;
	u32 mSizeOfUninitializedData;
	u32 mAddressOfEntryPoint;
	u32 mBaseOfCode;
	u32 mBaseOfData;
	u32 mImageBase;
	u32 mSectionAlignment;
	u32 mFileAlignment;
	u16 mMajorOperatingSystemVersion;
	u16 mMinorOperatingSystemVersion;
	u16 mMajorImageVersion;
	u16 mMinorImageVersion;
	u16 mMajorSubsystemVersion;
	u16 mMinorSubsystemVersion;
	u32 mWin32VersionValue;
	u32 mSizeOfImage;
	u32 mSizeOfHeaders;
	u32 mCheckSum;
	u16 mSubsystem;
	u16 mDllCharacteristics;
	u32 mSizeOfStackReserve;
	u32 mSizeOfStackCommit;
	u32 mSizeOfHeapReserve;
	u32 mSizeOfHeapCommit;
	u32 mLoaderFlags;
	u32 mNumberOfRvaAndSizes;
};

struct Pe32SectionHeader {
	char name[8];
	u32 virtualSize;
	u32 virtualAddress;
	u32 sizeOfRawData;
	u32 pointerToRawData;
	u32 pointerToRelocations;
	u32 pointerToLinenumbers;
	u16 numberOfRelocations;
	u16 numberOfLinenumbers;
	u32 characteristics;
};

class ExeFile {
    public:
		u8 *buffer;
		PeHeader header;
		std::map<std::string, Pe32SectionHeader> sections;
		ExeFile(u8 *buffer) {
			this->buffer = buffer;
			this->header = GetPEHeader();
			this->sections = ParseSectionHeaders();
		}
		~ExeFile() {
			delete[] buffer;
		}

		bool ContainsSection(std::string section_name);
		PeHeader GetPEHeader();
		Pe32OptionalHeader GetPEOptionalHeader();
		Pe32SectionHeader* GetSectionHeader(std::string target_section);
		std::map<std::string, Pe32SectionHeader> ParseSectionHeaders();

        static bool SigCheck(u8 *buffer);
};
