#include <stdio.h>
#include "ArchiveFormats/HSP/hsp.h"

int main() {
    HSPArchive *arc = new HSPArchive();

    auto [buffer, size] = arc->open("./eXceed3.exe");

    uint32_t attempt = arc->TryOpen(buffer, size);

    // printf("0x%x\n", attempt);

    // bool signature_check = ExeFile::SignatureCheck(buffer);
    // PeHeader header = ExeFile::GetPEHeader(buffer);
    // Pe32OptionalHeader optional_header = ExeFile::GetPEOptionalHeader(buffer);

    // printf("Signature check: %s\n", signature_check ? "Valid PE32 Executable!" : "Not a PE32 Executable!");

    return 0;
}