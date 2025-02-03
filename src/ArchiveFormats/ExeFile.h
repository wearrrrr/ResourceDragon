#include <cstdint>
#include <string>
#include <bit>

template<typename T = void>
static T* based_pointer(void* base, size_t offset) {
    return (T*)(uintptr_t(base) + offset);
}

static inline constexpr uint32_t PackUInt32(uint8_t c1, uint8_t c2 = 0, uint8_t c3 = 0, uint8_t c4 = 0) {
    return c4 << 24 | c3 << 16 | c2 << 8 | c1;
}
static inline constexpr uint32_t PackUInt(uint8_t c1, uint8_t c2 = 0, uint8_t c3 = 0, uint8_t c4 = 0) {
    return PackUInt32(c1, c2, c3, c4);
}

class ExeFile {
    public:
        static bool SignatureCheck(unsigned char *buffer, long size);

};