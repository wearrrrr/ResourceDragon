#include "pac.h"

ArchiveBase *PacFormat::TryOpen(u8 *buffer, u64 size, std::string file_name) {
    if (!CanHandleFile(buffer, size, file_name)) {
        return nullptr;
    }

    return new PacArchive({});
}

bool PacFormat::CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const {
    return ext == "pac";
}
