#include "ExeFile.h"

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

bool ExeFile::SignatureCheck(unsigned char *buffer, long size) {
	uint32_t pe_offset = *based_pointer<uint32_t>(buffer, 0x3C);
	printf("%04x\n", pe_offset);

    return false;
}