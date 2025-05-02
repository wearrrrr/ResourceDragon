#pragma once

#include <string>
#include <cstdint>
#include <vector>

typedef class XP3Crypt XP3Crypt;

struct Segment {
    bool IsCompressed;
    int64_t Offset;
    int64_t Size;
    int64_t PackedSize;
};

struct Entry {
    std::string name;
    uint32_t key;
    uint64_t offset;
    uint32_t size;

    uint32_t packedSize;
    bool isPacked;

    std::vector<Segment> segments;
    bool isEncrypted;
    XP3Crypt *crypt;
    uint32_t hash;
};
