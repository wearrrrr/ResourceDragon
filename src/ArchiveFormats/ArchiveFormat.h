#include <stdint.h>
#include <string>
#include <fstream>
#include <vector>

#include "ExeFile.h"

using std::string;

class ArchiveFormat {
    public:
        string tag = "?????";
        string description = "????? Resource Archive";

        uint32_t open(const char *path);
};