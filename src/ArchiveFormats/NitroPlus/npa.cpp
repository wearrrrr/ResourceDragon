#include "npa.h"

ArchiveBase *NPAFormat::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name)
{
    return nullptr;
}

bool NPAFormat::CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const
{
    return false;
}
