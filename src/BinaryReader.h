#pragma once

#include <util/int.h>
#include <vector>
#include <optional>
#include <string>

class BinaryReader {

public:
    const std::vector<u8>& data;
    size_t position;
    explicit BinaryReader(const std::vector<u8> &buffer) : data(buffer), position(0) {}

    std::optional<u8> peek() const {
        if (position >= data.size()) return std::nullopt;
        return data[position];
    }

    template <typename T>
    T& read() {
      size_t offset = position;
      position += sizeof(T);
      return *(T*)&data[offset];
    }

    std::string ReadChars(size_t count) {
        std::string result(data.begin() + position, data.begin() + position + count);
        position += count;
        return result;
    }
};
