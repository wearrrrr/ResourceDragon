#pragma once

#include <ArchiveFormat.h>
#include <Entry.h>
#include <cstdlib>
#include <util/memory.h>

enum Compression {
    None,
    Lzss,
    Huffman,
    Deflate,
    DeflateOrNone,
    None2,
    Zstd,
    ZstdOrNone,
};

class PacFormat : public ArchiveFormat {
public:
    PacFormat() {
        this->tag = "PAC";
        this->description = "NeXas PAC Archive.";
    };

    ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;
    bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override;
};

class PacArchive : public ArchiveBase {
public:
    PacArchive(const EntryMap &entries) : ArchiveBase(entries) {};
    ~PacArchive() = default;

    u8* OpenStream(const Entry *entry, u8 *buffer) override {
        return nullptr;
    };
};
