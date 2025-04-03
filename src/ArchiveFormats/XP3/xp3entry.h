#pragma once
#include <vector>
#include "../../GameRes/Entry.h"

typedef struct ICrypt ICrypt;

struct XP3Segment
{
    bool IsCompressed;
    long Offset;
    uint32_t Size;
    uint32_t PackedSize;
};

struct XP3Entry : PackedEntry {
    std::vector<XP3Segment> m_segments;
    bool m_isEncrypted;
    ICrypt *m_crypt;
    uint32_t m_hash;
};