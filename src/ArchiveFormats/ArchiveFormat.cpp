#include "ArchiveFormat.h"

uint32_t ArchiveFormat::open(const char *path)
{
    printf("Loading %s...\n", path);

    auto [buffer, size] = read_file_to_buffer<unsigned char>(path);

    printf("Loaded!\n");

    bool signature_check = ExeFile::SignatureCheck(buffer, size);

    printf("Signature check: %s\n", signature_check ? "true" : "false");

    return 0;
}