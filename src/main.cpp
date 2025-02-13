#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "ArchiveFormats/HSP/hsp.h"

namespace fs = std::filesystem;

int main() {
    HSPArchive *arc = new HSPArchive();
    auto [buffer, size] = arc->open("./eXceed3.exe");
    DPMArchive *opened_arc = arc->TryOpen(buffer, size);
    DPMEntry entry = opened_arc->entries.at(0);

    if (entry.offset + entry.size > size) {
        printf("Entry is out of bounds! This is very bad.\n");
        return 1;
    }

    fs::remove_all("decrypt/");
    fs::create_directory("decrypt");

    for (int i = 0; i < opened_arc->entries.size(); i++) {
        DPMEntry entry = opened_arc->entries.at(i);
        const char *data = opened_arc->OpenStream(entry, buffer);
        std::ofstream outFile("decrypt/" + entry.name, std::ios::binary);
        outFile.write((const char*)data, entry.size);
        outFile.close();
    }
    printf("Decrypted successfully!\n");
    
    return 0;
}