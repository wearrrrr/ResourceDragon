#include "zip.h"
#include "Entry.h"

ArchiveBase* ZipFormat::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) {
    zip_error_t err;
    zip_source_t *src_buf = zip_source_buffer_create(buffer, size, 0, &err);
    if (src_buf == nullptr) {
        Logger::error("Failed to create zip source: %s\n", zip_error_strerror(&err));
        return nullptr;
    }
    zip_t *za = zip_open_from_source(src_buf, ZIP_RDONLY, &err);
    if (za == nullptr) {
        zip_source_free(src_buf);
        Logger::error("Failed to open zip from buffer: %s\n", zip_error_strerror(&err));
        return nullptr;
    }

    std::unordered_map<std::string, Entry*> entries;

    zip_int64_t num_entries = zip_get_num_entries(za, 0);
    entries.rehash(num_entries);
    for (zip_uint64_t i = 0; i < (zip_uint64_t)num_entries; i++) {
        zip_stat_t stat;
        zip_stat_index(za, i, 0, &stat);

        if (stat.size <= 0) continue;

        auto entry = new Entry {
            .name = stat.name,
            .size = static_cast<uint32_t>(stat.size),
            .lastModified = stat.mtime,
            .packedSize = static_cast<uint32_t>(stat.comp_size),
        };

        zip_file_t *zf = zip_fopen_index(za, i, 0);
        if (zf) {
            std::vector<uint8_t> entry_data(stat.size);
            zip_int64_t bytes_read = zip_fread(zf, entry_data.data(), stat.size);
            if (bytes_read > 0) {
                entry->data = entry_data;
            }
            zip_fclose(zf);
        }

        entries.emplace(stat.name, entry);
    }

    zip_close(za);
    return new ZipArchive(std::move(entries));
}
