#pragma once

#include <ArchiveFormat.h>
#include <Entry.h>

#include <zip.h>

class ZipFormat : public ArchiveFormat {
    public:
        std::string_view tag = "ZIP";
        std::string_view description = "Zip Archive";

        std::vector<std::string> extensions = {"zip", "zipx", "z01", "zx01"};

        bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const override  {
            if (VectorHas(extensions, ext)) return true;

            return false;
        };

        ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) override;

        std::string_view GetTag() const override {
            return this->tag;
        };
        std::string_view GetDescription() const override {
            return this->description;
        }
};

class ZipArchive : public ArchiveBase {
public:
    EntryMap entries;
    zip_t *zip_arc;

    ZipArchive(EntryMap entries, zip_t *za) : entries(entries) {
        zip_arc = za;
    }

    u8* OpenStream(const Entry *entry, u8 *buffer) override {
        if (!entry) return nullptr;

        zip_file_t *zf = zip_fopen_index(zip_arc, entry->index, 0);
        if (!zf) {
            Logger::error("Failed to open file '%s' by index!", entry->name.c_str());
            return nullptr;
        }

        u8 *buf = (u8*)malloc(entry->size);
        if (!buf) {
            zip_fclose(zf);
            return nullptr;
        }

        zip_fread(zf, buf, entry->size);
        zip_fclose(zf);

        return buf;
    }

    EntryMapPtr GetEntries() override {
        EntryMapPtr entries;
        for (auto& entry : this->entries)
            entries[entry.first] = &entry.second;
        return entries;
    }

    ~ZipArchive() {
        this->entries.clear();
        zip_close(zip_arc);
    }
};
