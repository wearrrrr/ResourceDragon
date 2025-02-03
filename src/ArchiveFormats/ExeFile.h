#include <stdint.h>
#include <cstring>
#include <string>
#include <vector>
#include <span>

class ExeFile {
    public:
        static bool SignatureCheck(unsigned char byte1, unsigned char byte2);

};