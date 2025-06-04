#include "../ArchiveFormat.h"

struct BitReader {
    const unsigned char *buffer;
    size_t size;
    size_t position;
    unsigned char current_byte;
    unsigned char bitmask;
};

#define ENTRY_NAME_LENGTH 256

struct DatEntry {
    unsigned unk[2];
    unsigned checksum;
    unsigned size;
    unsigned data_offset;
    char name[ENTRY_NAME_LENGTH];
};

struct DatFile {
    BitReader reader;

    DatEntry *entries;
    unsigned entries_count;

    unsigned file_table_offset;

    unsigned char initialized;
};

class THDAT : public ArchiveFormat {
    std::string tag = "Touhou.DAT";
    std::string description = "Archive format for mainline Touhou games";
    uint32_t sig = PackUInt32('P', 'B', 'G', '3');

    std::vector<std::string> extensions = {".dat", ".DAT"};

    ArchiveBase *TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;
    ArchiveBase *TryOpenTH06(unsigned char *buffer, uint32_t size, std::string file_name);

    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override {
        if (ext == "dat" || ext == "DAT")
            return Read<uint32_t>(buffer, 0) == sig;

        return false;
    };
    std::string GetTag() const override {
        return this->tag;
    };
};

class THDATArchive : public ArchiveBase {
    public:
    DatFile dat;
    std::unordered_map<std::string, DatEntry> entries;

    THDATArchive(const DatFile &dat, std::unordered_map<std::string, DatEntry> entries) {
        this->dat = dat;
        this->entries = entries;
    }

    std::unordered_map<std::string, Entry*> GetEntries() override {
        std::unordered_map<std::string, Entry*> entry_map;
        for (auto &entry : this->entries) {
            Entry *rdEntry = new Entry {
                .name = entry.first,
                .size = entry.second.size
            };
            entry_map.emplace(entry.first, rdEntry);
        }
        return entry_map;
    };

    const char* OpenStream(const Entry *entry, unsigned char *buffer) override;
};
