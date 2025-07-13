#pragma once

#include <ArchiveFormat.h>

class NPKFormat : public ArchiveFormat {
    std::string_view tag = "NitroPlus.NPK";
    std::string_view description = "Nitro+ NPK Resource Archive";
    u32 sig = 0x324B504E; // NPK2;

    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override {
        if (ReadMagic<u32>(buffer) == sig) return true;

        return false;
    };
    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    std::string_view GetTag() const override {
        return this->tag;
    }
    std::string_view GetDescription() const override {
        return this->description;
    }
};
