#pragma once

#include "../ArchiveFormat.h"


class NPAFormat : public ArchiveFormat {
    std::string tag = "NitroPlus.NPA";
    std::string description = "Nitro+ Resource Archive";

    ArchiveBase* TryOpen(unsigned char *buffer, uint64_t size, std::string file_name) override;
    bool CanHandleFile(unsigned char *buffer, uint64_t size, const std::string &ext) const override;
    std::string GetTag() const override {
        return this->tag;
    }
};
