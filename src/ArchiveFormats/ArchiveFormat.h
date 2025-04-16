#pragma once

#include <stdint.h>

#include "ExeFile.h"
#include "../GameRes/Entry.h"
#include "../util/Logger.h"

class ArchiveBase {
    public:
        std::vector<Entry> entries;

        virtual const char* OpenStream(const Entry &entry, unsigned char *buffer) = 0;

        virtual ~ArchiveBase() = default;
};

class ArchiveFormat {
    public:
        std::string tag = "?????";
        std::string description = "????? Resource Archive";
        uint32_t sig = 0x00000000;

        uint32_t ReadUint32(unsigned char *buffer, uint64_t offset) {
            return *based_pointer<uint32_t>(buffer, offset);
        }

        std::string ReadString(unsigned char *buffer, uint64_t offset) {
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

        virtual bool CanHandleFile(unsigned char *buffer, uint32_t size, const std::string &ext) const = 0;
        virtual ArchiveBase* TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) = 0;
        virtual std::string getTag() const = 0;
};