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
    std::unordered_map<std::string, Entry*> entries;

    explicit ZipArchive(std::unordered_map<std::string, Entry*> entries) : entries(entries) {}

    const char* OpenStream(const Entry *entry, unsigned char *buffer) override {
        return (const char*)entry->data.data();
    }

    std::unordered_map<std::string, Entry*> GetEntries() override {
        return this->entries;
    }

    ~ZipArchive() {
        for (auto &pair : this->entries) {
            delete pair.second;
        }
        this->entries.clear();
    }
};
