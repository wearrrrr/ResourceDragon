/* These templates were made by zero318, all credit goes to them! :) */
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#define _MACRO_STR(arg) #arg
#define MACRO_STR(arg) _MACRO_STR(arg)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>

template <typename T = void>
[[nodiscard]] inline auto read_file_to_buffer(const char *path) {
  long file_size = 0;
  T *buffer = NULL;
  if (FILE *file = fopen(path, "rb")) {
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    if ((buffer = (T*)malloc(file_size))) {
      rewind(file);
      fread(buffer, file_size, 1, file);
    }
    fclose(file);
  }
  return std::make_pair(buffer, file_size);
}

template <typename T = void>
static T *based_pointer(void *base, size_t offset) {
  return (T *)(uintptr_t(base) + offset);
}

static inline constexpr uint16_t PackUInt16(uint8_t c1, uint8_t c2 = 0) {
  return c2 << 8 | c1;
}
static inline constexpr uint32_t PackUInt32(uint8_t c1, uint8_t c2 = 0, uint8_t c3 = 0, uint8_t c4 = 0) {
  return c4 << 24 | c3 << 16 | c2 << 8 | c1;
}
static inline constexpr uint32_t PackUInt(uint8_t c1, uint8_t c2 = 0, uint8_t c3 = 0, uint8_t c4 = 0) {
  return PackUInt32(c1, c2, c3, c4);
}
