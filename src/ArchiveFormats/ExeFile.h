#include <cstdint>
#include <string>

template<typename T = void>
static T* based_pointer(void* base, size_t offset) {
    return (T*)(uintptr_t(base) + offset);
}

class ExeFile {
    public:
        static bool SignatureCheck(unsigned char *buffer, long size);

};