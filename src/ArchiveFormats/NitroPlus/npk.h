#pragma once

#include <ArchiveFormat.h>

class NPKFormat : public ArchiveFormat {
    std::string tag = "NitroPlus.NPK";
    std::string description = "Nitro+ NPK Resource Archive";
    u32 sig = 0x324B504E; // NPK2;

    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override {
        if (ReadMagic<u32>(buffer) == sig) return true;

        return false;
    };
    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    std::string GetTag() const override {
        return this->tag;
    }
};
