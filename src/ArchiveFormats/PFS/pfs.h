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
};

class PFSArchive : public ArchiveBase {
    PFSFormat *pfs_fmt;
    std::unordered_map<std::string, Entry> entries;
    std::vector<u8> key;
    public:
        PFSArchive(const std::unordered_map<std::string, Entry> &entries) {
            this->entries = entries;
        }
        PFSArchive(PFSFormat *arc_fmt, const std::unordered_map<std::string, Entry> &entries, std::vector<u8> key) {
            this->pfs_fmt = arc_fmt;
            this->entries = entries;
            this->key = key;
        }
        std::unordered_map<std::string, Entry*> GetEntries() override {
            std::unordered_map<std::string, Entry*> entries;
            for (auto& entry : this->entries)
                entries[entry.first] = &entry.second;
            return entries;
        }
        const char* OpenStream(const Entry *entry, u8 *buffer) override;
};
