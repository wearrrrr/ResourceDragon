#include "../ArchiveFormat.h"
#include <unordered_map>

struct MPKEntry : Entry {
    uint32_t Id;
    int64_t Offset;
    bool Compressed;
    int64_t CompressedSize;
};

class MPKFormat : public ArchiveFormat {
    std::string tag = "NitroPlus.MPK";
    std::string description = "Nitro+ Resource Archive";
    uint32_t sig = 0x4B504D;

    ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;
    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override;
    std::string GetTag() const override {
        return this->tag;
    }
};

class MPKArchive : public ArchiveBase {
    std::unordered_map<std::string, MPKEntry> entries;
    public:
        MPKArchive(const std::unordered_map<std::string, MPKEntry> &entries) {
            this->entries = entries;
        }
        std::unordered_map<std::string, Entry*> GetEntries() override {
            std::unordered_map<std::string, Entry*> entriesMap;
            for (auto &entry : entries) {
                entriesMap.insert({entry.first, &entry.second});
            }
            return entriesMap;
        }
        const char* OpenStream(const Entry *entry, unsigned char *buffer) override;
};
