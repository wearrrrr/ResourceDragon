#include "npa.h"

ArchiveBase *NPAFormat::TryOpen(uint8_t *buffer, uint64_t size, std::string file_name)
{
    return nullptr;
}

bool NPAFormat::CanHandleFile(uint8_t *buffer, uint64_t size, const std::string &ext) const
{
    return false;
}
