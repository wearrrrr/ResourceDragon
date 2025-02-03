#include "ArchiveFormat.h"

std::pair<unsigned char*, long> ArchiveFormat::open(const char *path)
{
    printf("Loading %s...\n", path);
    return read_file_to_buffer<unsigned char>(path);
}