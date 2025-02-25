#include <stdint.h>
#include <stdexcept>

#include "ExeFile.h"
#include "../GameRes/Entry.h"

using std::string;

template <typename T = void>
[[nodiscard]] inline auto read_file_to_buffer(const char* path) {
    long file_size = 0;
    T* buffer = NULL;
    if (FILE* file = fopen(path, "rb")) {
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        if ((buffer = (T*)malloc(file_size))) {
            rewind(file);
            fread(buffer, file_size, 1, file);
        }
        fclose(file);
    }
    return std::make_pair(buffer, file_size);
}

class ArchiveBase {
    public:
        std::vector<Entry> entries;

        virtual const char* OpenStream(Entry entry, unsigned char *buffer) = 0;
        virtual std::vector<Entry> getEntries() = 0;
};

class ArchiveFormat {
    public:
        string tag = "?????";
        string description = "????? Resource Archive";
        uint32_t sig = 0x00000000;

        auto open(const char *path) {
            printf("Loading %s...\n", path);
            auto buffer = read_file_to_buffer<unsigned char>(path);
            printf("Loaded!\n");
            return buffer;
        };

        uint32_t ReadUint32(unsigned char *buffer, uint32_t offset) {
            return *based_pointer<uint32_t>(buffer, offset);
        }

        std::string ReadString(unsigned char *buffer, uint32_t offset, size_t read_amount) {
            std::string constructed = "";
            constructed.append(based_pointer<char>(buffer, offset));

            return constructed;
        }

        ExeFile* ConvertToExeFile(unsigned char *buffer) {
            return new ExeFile(buffer);
        }

        // Taken directly from GARbro GameRes->ArchiveFormat.cs
        bool IsSaneFileCount(uint32_t file_count) {
            return file_count > 0 && file_count < 0x40000;
        }

        virtual ~ArchiveFormat() = default;

        virtual bool CanHandleFile(unsigned char *buffer, uint32_t size) const = 0;
        virtual void* TryOpen(unsigned char *buffer, uint32_t size) = 0;
        virtual std::string getTag() const = 0;
};