#include "../ArchiveFormat.h"

class DPMArchive {
    public:
        uint8_t seed_1;
        uint8_t seed_2;
        DPMArchive() {
            
        };
        DPMArchive(uint32_t arc_key, size_t dpm_size) {
            seed_1 = ((((arc_key >> 16) & 0xFF) * (arc_key & 0xFF) / 3) ^ dpm_size);
        }
};

class HSPArchive : public ArchiveFormat {
    public:
        string tag = "HSP";
        string description = "Hot Soup Processor 3 Resource Archive";

        uint32_t DefaultKey = 0xAC52AE58;

        HSPArchive() {
            sig = 0x584D5044; // "DPMX"
        };

        DPMArchive* TryOpen(unsigned char *buffer, uint32_t size);
        
        uint32_t FindExeKey(ExeFile *exe, uint32_t dpmx_offset);
};