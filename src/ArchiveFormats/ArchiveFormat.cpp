#include "ArchiveFormat.h"
#include <span>
#include <iostream>

uint32_t ArchiveFormat::open(const char *path)
{
    printf("Loading %s...\n", path);

    std::ifstream input(path, std::ifstream::binary);

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

    printf("Loaded!\n");

    bool signature_check = ExeFile::SignatureCheck(buffer);

    printf("Signature check: %s\n", signature_check ? "true" : "false");

    return 0;
}