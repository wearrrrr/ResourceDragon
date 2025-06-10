#pragma once

#include "../ArchiveFormat.h"

class PFSFormat : public ArchiveFormat {
    std::string tag = "PFS";
    std::string description = "PFS Resource Archive";

    std::vector<std::string> extensions = {"pfs", "000", "001", "002", "003", "004", "005", "010"};

    ArchiveBase *OpenPF(unsigned char *buffer, uint64_t size, uint8_t version);

    ArchiveBase* TryOpen(unsigned char *buffer, uint64_t size, std::string file_name) override;
    bool CanHandleFile(unsigned char *buffer, uint64_t size, const std::string &ext) const override;
    std::string GetTag() const override {
        return this->tag;
    }
};

class PFSArchive : public ArchiveBase {
    PFSFormat *pfs_fmt;
    std::unordered_map<std::string, Entry> entries;
    std::vector<uint8_t> key;
    public:
        PFSArchive(const std::unordered_map<std::string, Entry> &entries) {
            this->entries = entries;
        }
        PFSArchive(PFSFormat *arc_fmt, const std::unordered_map<std::string, Entry> &entries, std::vector<uint8_t> key) {
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
        const char* OpenStream(const Entry *entry, unsigned char *buffer) override;
};
