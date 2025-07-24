#include "pbg.h"
#include <stdlib.h>


ArchiveBase *THDAT::TryOpenTH06(u8 *buffer, u64 size, std::string file_name) {
    ThArchive archive = {};
    std::unordered_map<std::string, ThEntry> entries;

    thOpenArchive(&archive, "tests/test.DAT");

    for (unsigned idx = 0; idx < archive.entries_count; ++idx) {
        entries.emplace(std::string(archive.entries[idx].name), archive.entries[idx]);
    }

    return new THDATArchive(archive, entries);
}

ArchiveBase *THDAT::TryOpen(u8 *buffer, u64 size, std::string file_name) {
    return TryOpenTH06(buffer, size, file_name);
}
