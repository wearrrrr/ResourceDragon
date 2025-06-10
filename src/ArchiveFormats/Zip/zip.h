#pragma once

#include <ArchiveFormat.h>
#include <Entry.h>

#include <zip.h>

class ZipFormat : public ArchiveFormat {
    public:
        std::string tag = "ZIP";
        std::string description = "Zip Archive";

        std::vector<std::string> extensions = {"zip", "zipx", "z01", "zx01"};

        bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override  {
            if (VectorHas(extensions, ext)) return true;

            return false;
        };

        ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;

        std::string GetTag() const override {
            return this->tag;
        };
};

class ZipArchive : public ArchiveBase {
public:
    std::unordered_map<std::string, Entry> entries;
    zip_t *zip_arc;

    ZipArchive(std::unordered_map<std::string, Entry> entries, zip_t *za) : entries(entries) {
        zip_arc = za;
    }

    const char* OpenStream(const Entry *entry, unsigned char *buffer) override {
        if (!entry) return nullptr;

        zip_file_t *zf = zip_fopen_index(zip_arc, entry->index, 0);
        if (!zf) {
            Logger::error("Failed to open file '%s' by index!", entry->name.c_str());
            return nullptr;
        }

        char *buf = (char*)malloc(entry->size);
        if (!buf) {
            zip_fclose(zf);
            return nullptr;
        }

        zip_fread(zf, buf, entry->size);
        zip_fclose(zf);

        return buf;
    }

    std::unordered_map<std::string, Entry*> GetEntries() override {
        std::unordered_map<std::string, Entry*> entries;
        for (auto& entry : this->entries)
            entries[entry.first] = &entry.second;
        return entries;
    }

    ~ZipArchive() {
        this->entries.clear();
        zip_close(zip_arc);
    }
};
