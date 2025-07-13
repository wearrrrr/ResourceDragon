#pragma once

#include <ArchiveFormat.h>


class NPAFormat : public ArchiveFormat {
    std::string_view tag = "NitroPlus.NPA";
    std::string_view description = "Nitro+ Resource Archive";

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
    std::string_view GetTag() const override {
        return this->tag;
    }
    std::string_view GetDescription() const override {
        return this->description;
    }
};
