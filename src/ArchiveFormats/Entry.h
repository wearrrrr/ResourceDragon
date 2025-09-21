#pragma once

#include "util/int.h"
#include <string>
#include <vector>
#include <ctime>

typedef class XP3Crypt XP3Crypt;

struct Segment {
    bool IsCompressed;
    u64 Offset;
    i64 Size;
    u64 PackedSize;
};

struct Entry {
    std::string name;
    u32 key;
    u64 offset;
    u64 size;
    time_t lastModified;

    u64 packedSize;
    bool isPacked;
    u64 index;

    std::vector<Segment> segments;
    bool isEncrypted;
    XP3Crypt *crypt;
    u64 hash;

    // Only used in some archive formats where the alternative would be passing around a reference to ArchiveFormat*, which is very annoying.
    std::vector<u8> data;
};
