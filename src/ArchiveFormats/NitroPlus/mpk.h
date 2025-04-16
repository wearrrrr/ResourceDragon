#include "../ArchiveFormat.h"

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