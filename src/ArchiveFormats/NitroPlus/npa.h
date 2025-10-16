#pragma once

#include <ArchiveFormat.h>


class NPAFormat : public ArchiveFormat {
public:
    NPAFormat() {
        this->tag = "NitroPlus.NPA";
        this->description = "Nitro+ Resource Archive";
    }

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
};
