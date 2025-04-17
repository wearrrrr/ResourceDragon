#include "../ArchiveFormat.h"

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
    std::string getTag() const override {
        return this->tag;
    }
};

class MPKArchive : public ArchiveBase {
    std::vector<MPKEntry> entries;
    public:
        MPKArchive(const std::vector<MPKEntry> &entries) {
            this->entries = entries;
        }
        std::vector<Entry*> GetEntries() override {
            std::vector<Entry*> basePtrs;
            for (auto& entry : entries)
                basePtrs.push_back(&entry);
            return basePtrs;
        }
        const char* OpenStream(const Entry *entry, unsigned char *buffer) override;
};