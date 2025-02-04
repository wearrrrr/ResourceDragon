#include "../ArchiveFormat.h"

class HSPArchive : public ArchiveFormat {
    public:
        string tag = "HSP";
        string description = "Hot Soup Processor 3 Resource Archive";

        uint32_t DefaultKey = 0xAC52AE58;

        HSPArchive() {
            sig = 0x584D5044; // "DPMX"
        };

        uint32_t TryOpen(unsigned char *buffer, uint32_t size);
        
        uint32_t FindExeKey(unsigned char *buffer, uint32_t dpmx_offset);
};