#pragma once

#include <string>
#include <cstdint>
#include <vector>

typedef class XP3Crypt XP3Crypt;

struct Segment {
    bool IsCompressed;
    uint64_t Offset;
    int64_t Size;
    uint64_t PackedSize;
};

struct Entry {
    std::string name;
    uint32_t key;
    uint64_t offset;
    uint64_t size;
    time_t lastModified;

    uint32_t packedSize;
    bool isPacked;
    uint64_t index;

    std::vector<Segment> segments;
    bool isEncrypted;
    XP3Crypt *crypt;
    uint64_t hash;

    // Only used in some archive formats where the alternative would be passing around a reference to ArchiveFormat*, which is very annoying.
    std::vector<uint8_t> data;
};
