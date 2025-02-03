#include "../ArchiveFormat.h"

class HSPArchive : public ArchiveFormat {
    public:
        string tag = "HSP";
        string description = "Hot Soup Processor 3 Resource Archive";
        uint32_t sig = 0x584D5044; // "DPMX"

        HSPArchive() {};
};