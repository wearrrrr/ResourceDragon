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

bool ExeFile::SignatureCheck(unsigned char byte1, unsigned char byte2) {
    if (byte1 == 0x4D && byte2 == 0x5A) {
        return true;
    }
    return false;
}