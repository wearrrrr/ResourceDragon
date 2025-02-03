#include <stdint.h>

#include "ExeFile.h"

using std::string;

template <typename T = void>
[[nodiscard]] inline auto read_file_to_buffer(const char* path) {
    long file_size = 0;
    T* buffer = NULL;
    if (FILE* file = fopen(path, "rb")) {
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

class ArchiveFormat {
    public:
        string tag = "?????";
        string description = "????? Resource Archive";

        std::pair<unsigned char*, long> open(const char *path);
};