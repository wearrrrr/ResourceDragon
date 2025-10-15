#pragma once

#include <ArchiveFormat.h>

class MPKFormat : public ArchiveFormat {
    std::string tag = "NitroPlus.MPK";
    std::string description = "Nitro+ MPK Resource Archive";
    u32 sig = 0x4B504D;

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
    std::string GetTag() const override {
        return this->tag;
    }
    std::string GetDescription() const override {
        return this->description;
    }
};

class MPKArchive : public ArchiveBase {
    public:
        MPKArchive(const EntryMap &entries) : ArchiveBase(entries) {};

        u8* OpenStream(const Entry *entry, u8 *buffer) override;
};
