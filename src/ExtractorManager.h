#include <vector>

class ExtractorManager {
private:
    std::vector<std::unique_ptr<ArchiveFormat>> formats;

public:
    void registerFormat(std::unique_ptr<ArchiveFormat> format) {
        formats.push_back(std::move(format));
    }

    ArchiveFormat* getExtractorFor(unsigned char *buffer, uint32_t size) {
        for (const auto& format : formats) {
            /* TODO: 
                this is bad! we should not be returning the first one that happens to meet the criteria
                we should instead return a vector or something and try to extract each one
                or, we can make absolutely certain that ArchiveFormat::CanHandleFile ONLY returns the one we need
                But that seems like a lot of redundant work! */
            if (format->CanHandleFile(buffer, size)) {
                return format.get();
            }
        }
        return nullptr;
    }
};
