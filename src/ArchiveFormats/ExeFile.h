#pragma once

#include <cstdint>
#include <string>
#include <map>

#include "../zero_templates.h" // IWYU pragma: keep

struct PeHeader {
	uint32_t mMagic; // PE\0\0
	uint16_t mMachine;
	uint16_t mNumberOfSections;
	uint32_t mTimeDateStamp;
	uint32_t mPointerToSymbolTable;
	uint32_t mNumberOfSymbols;
	uint16_t mSizeOfOptionalHeader;
	uint16_t mCharacteristics;
};

struct Pe32OptionalHeader {
	uint16_t mMagic; // 0x010b - PE32, 0x020b - PE32+ (64 bit)
	uint8_t  mMajorLinkerVersion;
	uint8_t  mMinorLinkerVersion;
	uint32_t mSizeOfCode;
	uint32_t mSizeOfInitializedData;
	uint32_t mSizeOfUninitializedData;
	uint32_t mAddressOfEntryPoint;
	uint32_t mBaseOfCode;
	uint32_t mBaseOfData;
	uint32_t mImageBase;
	uint32_t mSectionAlignment;
	uint32_t mFileAlignment;
	uint16_t mMajorOperatingSystemVersion;
	uint16_t mMinorOperatingSystemVersion;
	uint16_t mMajorImageVersion;
	uint16_t mMinorImageVersion;
	uint16_t mMajorSubsystemVersion;
	uint16_t mMinorSubsystemVersion;
	uint32_t mWin32VersionValue;
	uint32_t mSizeOfImage;
	uint32_t mSizeOfHeaders;
	uint32_t mCheckSum;
	uint16_t mSubsystem;
	uint16_t mDllCharacteristics;
	uint32_t mSizeOfStackReserve;
	uint32_t mSizeOfStackCommit;
	uint32_t mSizeOfHeapReserve;
	uint32_t mSizeOfHeapCommit;
	uint32_t mLoaderFlags;
	uint32_t mNumberOfRvaAndSizes;
};

struct Pe32SectionHeader {
	char name[8];
	uint32_t virtualSize;
	uint32_t virtualAddress;
	uint64_t sizeOfRawData;
	uint32_t pointerToRawData;
	uint32_t pointerToRelocations;
	uint32_t pointerToLinenumbers;
	uint16_t numberOfRelocations;
	uint16_t numberOfLinenumbers;
	uint32_t characteristics;
};

class ExeFile {
    public:
		unsigned char *buffer;
		PeHeader header;
		std::map<std::string, Pe32SectionHeader> sections;
		ExeFile(unsigned char *buffer) {
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

        static bool SigCheck(unsigned char *buffer);
};
