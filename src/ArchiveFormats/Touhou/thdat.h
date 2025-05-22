#include "../ArchiveFormat.h"

class THDAT : public ArchiveFormat {
    std::string tag = "Touhou.DAT";
    std::string description = "Archive format for mainline Touhou games";
    uint32_t sig = PackUInt32('P', 'B', 'G', '3');

    std::vector<std::string> extensions = {".dat", ".DAT"};

    ArchiveBase *TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) override;

    bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const override {
        if (ext == "dat" || ext == "DAT")
            return Read<uint32_t>(buffer, 0) == sig;

        return false;
    };
    std::string GetTag() const override {
        return this->tag;
    };
};
