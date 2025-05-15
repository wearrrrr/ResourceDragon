#include "thdat.h"

ArchiveBase *TryOpenTH06(unsigned char *buffer, uint32_t size, std::string file_name) {


    return nullptr;
}

ArchiveBase *THDAT::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) {


    return TryOpenTH06(buffer, size, file_name);
}
