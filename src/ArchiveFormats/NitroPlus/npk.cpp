#include "npk.h"

ArchiveBase *NPKFormat::TryOpen(u8 *buffer, u64 size, std::string file_name) {
    u32 count = Read<u32>(buffer, 0x18);
    if (!IsSaneFileCount(count)) return nullptr;
    Logger::log("%d", count);

    return nullptr;
};
