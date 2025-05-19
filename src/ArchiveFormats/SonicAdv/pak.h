#include "../ArchiveFormat.h"

class SAPakFormat : public ArchiveFormat {
    std::string tag = "SonicAdv.PAK";
    std::string description = "Sonic Adventure Resource Archive";
    uint32_t sig = PackUInt32(0x01, 'p', 'a', 'k');

    std::vector<std::string> extensions = {"pak"};

    ArchiveBase *TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;

    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &_ext) const override {
        return Read<uint32_t>(buffer, 0) == sig;
    };
    std::string getTag() const override {
        return this->tag;
    };
};

class SAPakArchive : public ArchiveBase {
    public:
        std::unordered_map<std::string, Entry> entries;
        SAPakArchive(const std::unordered_map<std::string, Entry> &entries) {
            this->entries = entries;
        };
        // std::vector<Entry*> GetEntries() override {
        //     std::vector<Entry*> entryList;
        //     for (auto& entry : entries)
        //         entryList.push_back(&entry);
        //     return entryList;
        // }
        std::unordered_map<std::string, Entry*> GetEntries() override {
            std::unordered_map<std::string, Entry*> entries;
            for (auto& entry : this->entries)
                entries[entry.first] = &entry.second;
            return entries;
        }
        const char *OpenStream(const Entry *entry, unsigned char *buffer) override;


};
