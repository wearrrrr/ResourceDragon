#include "pfs.h"

ArchiveBase *PFSArchive::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name)
{
    return nullptr;
}

bool PFSArchive::CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const
{
    if (std::find(extensions.begin(), extensions.end(), ext) == extensions.end()) {
        return false;
    }

    if (*based_pointer<uint16_t>(buffer, 0) == PackUInt16('p', 'f')) {
        Logger::log("the j");
        return true;
    }

    return false;
}
