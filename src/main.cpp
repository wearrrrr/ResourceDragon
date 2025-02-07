#include <stdio.h>
#include "ArchiveFormats/HSP/hsp.h"

int main() {
    HSPArchive *arc = new HSPArchive();

    auto [buffer, size] = arc->open("./eXceed3.exe");

    DPMArchive *attempt = arc->TryOpen(buffer, size);
    
    return 0;
}