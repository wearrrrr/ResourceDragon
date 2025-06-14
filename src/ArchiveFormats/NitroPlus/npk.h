#pragma once

#include <ArchiveFormat.h>

class NPKFormat : public ArchiveFormat {
    std::string tag = "NitroPlus.NPK";
    std::string description = "Nitro+ NPK Resource Archive";
    uint32_t sig = 0x324B504E; // NPK2;

    bool CanHandleFile(uint8_t *buffer, uint64_t size, const std::string &ext) const override {
        if (ReadMagic<uint32_t>(buffer) == sig) return true;

        return false;
    };
    ArchiveBase* TryOpen(uint8_t *buffer, uint64_t size, std::string file_name) override;
    std::string GetTag() const override {
        return this->tag;
    }
};
