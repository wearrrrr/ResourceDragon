#pragma once

#include <algorithm>
#include "../ArchiveFormat.h"


class NPA : public ArchiveFormat {
    std::string tag = "NPP";
    std::string description = "Nitro+ Resource Archive";

    ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;
    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override;
    std::string getTag() const override {
        return this->tag;
    }
};