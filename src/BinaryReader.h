#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include <string>

class BinaryReader {

public:
    const std::vector<uint8_t>& data;
    size_t position;
    explicit BinaryReader(const std::vector<uint8_t>& buffer) : data(buffer), position(0) {}

    std::optional<uint8_t> peek() const {
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
