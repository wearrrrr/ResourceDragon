#include "zip.h"
#include "Entry.h"
#include <memory>
#include <zip.h>

zip_t *OpenZipFromFile(std::string file_name) {
    int err;
    zip_t *arc;
    if ((arc = zip_open(file_name.c_str(), 0, &err)) == nullptr) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        Logger::error("Zip: Failed to open archive %s: %s", file_name.c_str(), zip_error_strerror(&error));
    }
    return arc;
}

zip_t *OpenZipFromBuffer(unsigned char *buffer, uint64_t size) {
    zip_error_t err;
    std::unique_ptr<unsigned char[]> heap_buffer(new unsigned char[size]);
    memcpy(heap_buffer.get(), buffer, size);

    zip_source_t *src_buf = zip_source_buffer_create(heap_buffer.get(), size, ZIP_SOURCE_FREE, &err);
    if (src_buf == nullptr) {
        zip_error_init(&err);
        Logger::error("Failed to create zip source! %s\n", zip_error_strerror(&err));
        return nullptr;
    }
    heap_buffer.release();

    zip_t *za = zip_open_from_source(src_buf, ZIP_RDONLY, &err);
    if (za == nullptr) {
        zip_source_free(src_buf);
        zip_error_init(&err);
        Logger::error("Failed to open zip from buffer: %s\n", zip_error_strerror(&err));
        return nullptr;
    }

    return za;
};

ArchiveBase* ZipFormat::TryOpen(unsigned char *buffer, uint64_t size, std::string file_name) {
    std::unique_ptr<unsigned char[]> heap_buffer(new unsigned char[size]);
    memcpy(heap_buffer.get(), buffer, size);

    zip_t *za;

    if (size > 1000000000) {
        za = OpenZipFromFile(file_name);
    } else {
        za = OpenZipFromBuffer(buffer, size);
    }

    std::unordered_map<std::string, Entry> entries;
    zip_int64_t num_entries = zip_get_num_entries(za, 0);
    entries.rehash(num_entries);
    for (zip_uint64_t i = 0; i < (zip_uint64_t)num_entries; i++) {
        zip_stat_t stat;
        zip_stat_index(za, i, 0, &stat);
        if (stat.size <= 0) continue;

        Entry entry = {
            .name = std::string(stat.name),
            .size = static_cast<uint32_t>(stat.size),
            .lastModified = stat.mtime,
            .packedSize = static_cast<uint32_t>(stat.comp_size),
            .index = i
        };

        entries.emplace(entry.name, entry);
    }

    return new ZipArchive(entries, za);
}
