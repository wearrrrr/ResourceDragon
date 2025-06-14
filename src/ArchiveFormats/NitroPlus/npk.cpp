#include "npk.h"

ArchiveBase *NPKFormat::TryOpen(uint8_t *buffer, uint64_t size, std::string file_name) {
    uint32_t count = Read<uint32_t>(buffer, 0x18);
    if (!IsSaneFileCount(count)) return nullptr;
    Logger::log("%d", count);

    return nullptr;
};
