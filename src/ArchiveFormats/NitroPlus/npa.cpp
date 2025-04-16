#include "npa.h"

ArchiveBase *NPA::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name)
{
    return nullptr;
}

bool NPA::CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const
{
    return false;
}
