#include <stdint.h>
#include <cstring>
#include <string>
#include <vector>
#include <span>

class ExeFile {
    public:
        static bool SignatureCheck(std::vector<unsigned char> buffer);

};