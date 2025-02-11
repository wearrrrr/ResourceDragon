#include "../ArchiveFormat.h"
#include "../../GameRes/Entry.h"

struct DPMEntry : Entry {
    uint32_t key;
};

// TODO: Make generic ArchiveFile class to inherit from, so that I don't have to set variables on this class directly.
class DPMArchive {
    public:
        std::vector<DPMEntry> entries;
        uint32_t arc_key;
        size_t dpm_size;
        uint8_t seed_1;
        uint8_t seed_2;
        DPMArchive() {
            seed_1 = 0xAA;
            seed_2 = 0x55;
        };
        DPMArchive(std::vector<DPMEntry> &entries, uint32_t arc_key, size_t dpm_size) {
            seed_1 = ((((arc_key >> 16) & 0xFF) * (arc_key & 0xFF) / 3) ^ dpm_size);
            seed_2 = ((((arc_key >> 8)  & 0xFF) * ((arc_key >> 24) & 0xFF) / 5) ^ dpm_size ^ 0xAA);
            this->entries = entries;
            this->arc_key = arc_key;
            this->dpm_size = dpm_size;
        };
        void DecryptEntry(unsigned char *data, uint32_t data_size, uint32_t entry_key) {
            uint8_t s1 = 0x55;
            uint8_t s2 = 0xAA;
            s1 = (seed_1 + ((entry_key >> 16) ^ (entry_key + s1)));
            s2 = (seed_2 + ((entry_key >> 24) ^ ((entry_key >> 8) + s2)));
            uint8_t val = 0;
            for (int i = 0; i < data_size; i++) {
                val += (s1 ^ (data[i] - s2));
                data[i] = val;
            }
            return;
        };
};

class HSPArchive : public ArchiveFormat {
    public:
        string tag = "HSP";
        string description = "Hot Soup Processor 3 Resource Archive";

        uint32_t DefaultKey = 0xAC52AE58;

        HSPArchive() {
            sig = PackUInt('D', 'P', 'M', 'X');
        };

        DPMArchive* TryOpen(unsigned char *buffer, uint32_t size);
        
        uint32_t FindExeKey(ExeFile *exe, uint32_t dpmx_offset);
};