#pragma once

#include <ArchiveFormat.h>

class PFSFormat : public ArchiveFormat {
    std::string tag = "PFS";
    std::string description = "PFS Resource Archive";

    std::vector<std::string> extensions = {"pfs", "000", "001", "002", "003", "004", "005", "010"};

    ArchiveBase *OpenPF(u8 *buffer, u64 size, u8 version);

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
    std::string GetTag() const override {
        return this->tag;
    }
    std::string GetDescription() const override {
        return this->description;
    }
};

class PFSArchive : public ArchiveBase {
    PFSFormat *pfs_fmt;
    std::vector<u8> key;
    public:
        PFSArchive(const EntryMap &entries) : ArchiveBase(entries) {};
        PFSArchive(PFSFormat *arc_fmt, const EntryMap &entries, std::vector<u8> key) : ArchiveBase(entries) {
            this->pfs_fmt = arc_fmt;
            this->key = key;
        }
        u8* OpenStream(const Entry *entry, u8 *buffer) override;
};
