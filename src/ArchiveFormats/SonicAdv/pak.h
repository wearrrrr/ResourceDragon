#include <ArchiveFormat.h>

class SAPakFormat : public ArchiveFormat {
    std::string_view tag = "SonicAdv.PAK";
    std::string_view description = "Sonic Adventure Resource Archive";
    u32 sig = PackUInt32(0x01, 'p', 'a', 'k');

    std::vector<std::string> extensions = {"pak"};

    ArchiveBase *TryOpen(u8 *buffer, u64 size, std::string file_name) override;

    bool CanHandleFile(u8 *buffer, u64 size, const std::string &_ext) const override {
        return Read<u32>(buffer, 0) == sig;
    };
    std::string_view GetTag() const override {
        return this->tag;
    };
    std::string_view GetDescription() const override {
        return this->description;
    }
};

class SAPakArchive : public ArchiveBase {
    public:
        EntryMap entries;
        SAPakArchive(const EntryMap &entries) {
            this->entries = entries;
        };
        EntryMapPtr GetEntries() override {
            EntryMapPtr entries;
            for (auto& entry : this->entries)
                entries[entry.first] = &entry.second;
            return entries;
        }
        u8* OpenStream(const Entry *entry, u8 *buffer) override;


};
