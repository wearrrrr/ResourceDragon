#pragma once

#include <algorithm>
#include "../../sha1.h"
#include "../ArchiveFormat.h"

class PFSFormat : public ArchiveFormat {
    std::string tag = "PFS";
    std::string description = "PFS Resource Archive";

    std::vector<std::string> extensions = {"pfs", "000", "001", "002", "003", "004", "005", "010"};

    ArchiveBase *OpenPF(unsigned char *buffer, uint32_t size, uint8_t version);

    ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;
    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override;
    std::string getTag() const override {
        return this->tag;
    }
};

class PFSArchive : public ArchiveBase {
    PFSFormat *pfs_fmt;
    std::vector<Entry> entries;
    std::vector<uint8_t> key;
    public:
        PFSArchive(const std::vector<Entry> &entries) {
            this->entries = entries;
        }
        PFSArchive(PFSFormat *arc_fmt, const std::vector<Entry> &entries, std::vector<uint8_t> key) {
            this->pfs_fmt = arc_fmt;
            this->entries = entries;
            this->key = key;
        }
        std::vector<Entry*> GetEntries() override {
            std::vector<Entry*> basePtrs;
            for (auto& entry : entries)
                basePtrs.push_back(&entry);
            return basePtrs;
        }
        const char* OpenStream(const Entry *entry, unsigned char *buffer) override;
};