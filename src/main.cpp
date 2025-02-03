#include <stdio.h>
#include "ArchiveFormats/HSP/hsp.h"

int main() {
    HSPArchive *arc = new HSPArchive();

    auto [buffer, size] = arc->open("./eXceed3.exe");
    printf("Loaded!\n");

    bool signature_check = ExeFile::SignatureCheck(buffer, size);

    printf("Signature check: %s\n", signature_check ? "Valid PE32 Executable!" : "Not a PE32 Executable!");

    return 0;
}