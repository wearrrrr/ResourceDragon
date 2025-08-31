#pragma once

#include <optional>
#include <string>
#include <vector>

#include <util/int.h>

class BinaryReader {
private:
  const std::vector<u8> &data;

public:
  size_t position;
  explicit BinaryReader(const std::vector<u8> &buffer)
      : data(buffer), position(0) {}

  std::optional<u8> peek() const {
    if (position >= data.size())
      return std::nullopt;
    return data[position];
  }

  template <typename T>
  T &read() {
    size_t offset = position;
    position += sizeof(T);
    return *(T *)&data[offset];
  }

  std::string ReadChars(size_t count) {
    std::string result(data.begin() + position, data.begin() + position + count);
    position += count;
    return result;
  }
};
