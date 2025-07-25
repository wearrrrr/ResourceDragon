#include "hash.h"
#include <utility>

std::vector<u8> Hash::sha1(const u8 *data, usize len) {
    std::vector<u8> hash(SHA_DIGEST_LENGTH);
    SHA1(data, len, hash.data());
    return hash;
};

std::vector<u8> Hash::md5(const u8 *data, usize len) {
    std::unreachable();
}
