#pragma once

#include <limits>
#include <unordered_map>
#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

#include "../ArchiveFormat.h"
#include "../../BinaryReader.h"
#include "Crypt/Crypt.h"
#include "zlib.h"



enum XP3HeaderType {
    XP3_HEADER_UNPACKED = 0,
    XP3_HEADER_PACKED = 1,
};

class XP3Format : public ArchiveFormat {
    public:
        std::string tag = "XP3";
        std::string description = "XP3 Archive Format";

        uint32_t sig = 0x0d335058; // "XP3\0D"

        std::vector<std::string> extensions = {"xp3", "exe"};

        static constexpr unsigned char xp3_header[] = {
            0x58, 0x50, 0x33, 0x0d, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01
        };

        ArchiveBase *TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;

        std::string getTag() const override {
            return this->tag;
        };

        bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override {
            return (size > 0x10 && memcmp(buffer, xp3_header, sizeof(xp3_header)) == 0);
        };
};

class XP3Archive : public ArchiveBase {
    public:
        XP3Archive(std::vector<Entry> entries) {
            this->entries = entries;
        };

        const char *OpenStream(const Entry &entry, unsigned char *buffer) override;
        std::vector<Entry> GetEntries() override;
};