#include <stdio.h>
#include <fstream>
#include <cstring>
#include "ArchiveFormats/HSP/hsp.h"

int main() {
    HSPArchive *arc = new HSPArchive();
    auto [buffer, size] = arc->open("./eXceed3.exe");
    DPMArchive *opened_arc = arc->TryOpen(buffer, size);
    DPMEntry entry = opened_arc->entries.at(1);

    if (entry.offset + entry.size > size) {
        printf("Entry is out of bounds! This is very bad.\n");
        return 1;
    }

    unsigned char *data = new unsigned char[entry.size];
    std::memcpy(data, buffer + entry.offset, entry.size);

    printf("Offset: 0x%x\n", *based_pointer<uint32_t>(buffer, entry.offset));

    opened_arc->DecryptEntry(data, entry.size, entry.key);

    std::ofstream outFile("test.bmp", std::ios::binary);
    outFile.write((const char*)data, entry.size);
    outFile.close();
    printf("Modified entry saved successfully.\n");
    
    return 0;
}