#include "pbg.h"
#include <stdlib.h>


ArchiveBase *THDAT::TryOpenTH06(u8 *buffer, u64 size, std::string file_name) {
    ThArchive archive = {};
    std::unordered_map<std::string, ThEntry> entries;

    // Create a temporary file based on buffer
    FILE *temp_file = fopen("pbg_temp_file.dat", "wb");
    if (!temp_file) {
        Logger::log("Failed to create temporary file");
        return nullptr;
    }
    fwrite(buffer, 1, size, temp_file);
    fclose(temp_file);

    thOpenArchive(&archive, "pbg_temp_file.dat");

    for (unsigned idx = 0; idx < archive.entries_count; ++idx) {
        entries.emplace(std::string(archive.entries[idx].name), archive.entries[idx]);
    }

    // Free the temporary file
    remove("pbg_temp_file.dat");

    return new THDATArchive(archive, entries);
}

ArchiveBase *THDAT::TryOpen(u8 *buffer, u64 size, std::string file_name) {
    return TryOpenTH06(buffer, size, file_name);
}
