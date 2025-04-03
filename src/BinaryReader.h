#include <vector>
#include <cstdint>
#include <optional>
#include <iostream>

class BinaryReader {

public:
    const std::vector<uint8_t>& data;
    size_t position;
    explicit BinaryReader(const std::vector<uint8_t>& buffer) : data(buffer), position(0) {}

    std::optional<uint8_t> peek() const {
        if (position >= data.size()) return std::nullopt; // Indicate end of buffer
        return data[position]; 
    }

    template<typename T>
    T read() {
        T value;
        std::memcpy(&value, &data[position], sizeof(T));
        position += sizeof(T);
        return value;
    }

    std::string ReadChars(size_t count) {
        std::string result((const char*)(&data[position]), count);
        position += count;
        return result;
    }

    bool eof() const { return position >= data.size(); }
};