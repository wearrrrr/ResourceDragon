#pragma once

#include <string>

struct Entry {
    std::string name;
    std::string type;
    uint32_t key;
    uint32_t offset;
    uint32_t size;
};