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
	uint16_t mz_signature = *based_pointer<uint16_t>(buffer, 0x0);
	uint32_t pe_offset = *based_pointer<uint32_t>(buffer, 0x3C);
	uint32_t pe_signature = *based_pointer<uint32_t>(buffer, pe_offset);
	
	// printf("%04x\n", pe_offset);
	// printf("%04x\n", pe_signature);

	// MZ and PE in hex (reversed because endianness yayy)
    return mz_signature == 0x5A4D && pe_signature == 0x4550;
}