#include "mpk.h"

ArchiveBase *MPKFormat::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name)
{
    return nullptr;
}

bool MPKFormat::CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const
{
    if (ReadMagic(buffer) == sig) {
        return true;
    }
    return false;
}
