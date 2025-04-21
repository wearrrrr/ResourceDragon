#include "pfs.h"

ArchiveBase *PFSArchive::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name)
{
    if (!CanHandleFile(buffer, size, "")) return nullptr;

    return nullptr;
}

bool PFSArchive::CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const
{
    if (ext != "" && std::find(extensions.begin(), extensions.end(), ext) == extensions.end()) {
        return false;
    }

    if (*based_pointer<uint16_t>(buffer, 0) == PackUInt16('p', 'f')) {
        return true;
    }

    return false;
}
