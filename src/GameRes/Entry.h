#pragma once

#include <string>

typedef class XP3Crypt XP3Crypt;

struct Segment {
    bool IsCompressed;
    int64_t Offset;
    uint32_t Size;
    uint32_t PackedSize;
};

struct Entry {
    std::string name;
    std::string type;
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