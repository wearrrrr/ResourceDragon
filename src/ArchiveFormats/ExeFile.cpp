#include "ExeFile.h"

bool ExeFile::SignatureCheck(std::vector<unsigned char> buffer) {
    std::span<const unsigned char, sizeof(uint16_t)> exe_check(buffer.data(), sizeof(uint16_t));

    uint16_t value;
    memcpy(&value, exe_check.data(), sizeof(uint16_t));

    if (value == 0x5A4D) {
        return true;
    }
    return false;
}