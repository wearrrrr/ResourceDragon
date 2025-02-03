#include <stdio.h>
#include "ArchiveFormats/HSP/hsp.h"

int main() {
    HSPArchive *arc = new HSPArchive();

    arc->open("./eXceed3.exe");

    return 0;
}