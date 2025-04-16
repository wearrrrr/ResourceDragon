#include "../ArchiveFormat.h"

class MPKFormat : public ArchiveFormat {
    std::string tag = "MPK";
    std::string description = "Nitro+ Resource Archive";

    ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;
    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override;
    std::string getTag() const override {
        return this->tag;
    }
};