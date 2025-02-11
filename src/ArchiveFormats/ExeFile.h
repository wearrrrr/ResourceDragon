#include <cstdint>
#include <string>
#include <cstring>
#include <bit>
#include <vector>
#include <map>

struct PeHeader {
	uint32_t mMagic; // PE\0\0 or 0x00004550
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
	uint32_t sizeOfRawData;
	uint32_t pointerToRawData;
	uint32_t pointerToRelocations;
	uint32_t pointerToLinenumbers;
	uint16_t numberOfRelocations;
	uint16_t numberOfLinenumbers;
	uint32_t characteristics;
};

template<typename T = void>
static T* based_pointer(void* base, size_t offset) {
    return (T*)(uintptr_t(base) + offset);
}

static inline constexpr uint32_t PackUInt32(uint8_t c1, uint8_t c2 = 0, uint8_t c3 = 0, uint8_t c4 = 0) {
    return c4 << 24 | c3 << 16 | c2 << 8 | c1;
}
static inline constexpr uint32_t PackUInt(uint8_t c1, uint8_t c2 = 0, uint8_t c3 = 0, uint8_t c4 = 0) {
    return PackUInt32(c1, c2, c3, c4);
}

class ExeFile {
    public:
		unsigned char* raw_contents;
		PeHeader header;
		Pe32OptionalHeader opt_header;
		std::map<std::string, Pe32SectionHeader> sections;
		ExeFile(unsigned char *buffer) {
			raw_contents = buffer;
			sections = ParseSectionHeaders();
			header = GetPEHeader();
			opt_header = GetPEOptionalHeader();
		}

		bool ContainsSection(std::string section_name);
		PeHeader GetPEHeader();
		Pe32OptionalHeader GetPEOptionalHeader();
		Pe32SectionHeader* GetSectionHeader(std::string target_section);
		std::map<std::string, Pe32SectionHeader> ParseSectionHeaders();

        static bool SignatureCheck(unsigned char *buffer);
};