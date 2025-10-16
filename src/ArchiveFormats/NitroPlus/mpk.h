#pragma once

#include <ArchiveFormat.h>

class MPKFormat : public ArchiveFormat {
public:
    MPKFormat() {
        this->tag = "NitroPlus.MPK";
        this->description = "Nitro+ MPK Resource Archive";
    }

    u32 sig = 0x4B504D;

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
};

class MPKArchive : public ArchiveBase {
    public:
        MPKArchive(const EntryMap &entries) : ArchiveBase(entries) {};

        u8* OpenStream(const Entry *entry, u8 *buffer) override;
};
