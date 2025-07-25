#include <vector>
#include <util/int.h>
#include <openssl/sha.h>

namespace Hash {
    std::vector<u8> sha1(const u8 *data, usize len);
    std::vector<u8> md5(const u8 *data, usize len);
}
