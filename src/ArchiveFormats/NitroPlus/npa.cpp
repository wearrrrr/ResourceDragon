#include "npa.h"

ArchiveBase *NPAFormat::TryOpen(u8 *buffer, u64 size, std::string file_name)
{
    return nullptr;
}

bool NPAFormat::CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const
{
    return false;
}
