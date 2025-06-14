#pragma once

#include <stdint.h>

#include "ExeFile.h"
#include "Entry.h"
#include <cstring>
#include <unordered_map>
#include <algorithm>
#include "../util/Logger.h" // IWYU pragma: keep

template <typename T = std::string>
bool VectorHas(std::vector<T> vec, T item) {
    return std::find(vec.begin(), vec.end(), item) != vec.end();
}

class ArchiveBase {
    public:
        virtual const char* OpenStream(const Entry *entry, uint8_t *buffer) = 0;
        virtual std::unordered_map<std::string, Entry*> GetEntries() = 0;
        virtual ~ArchiveBase() = default;
};

class ArchiveFormat {
    public:
        std::string tag = "?????";
        std::string description = "????? Resource Archive";
        uint32_t sig = 0x00000000;

        size_t buffer_position = 0;

        template<typename T>
        T Read(uint8_t* buffer, size_t offset) const {
            return *based_pointer<T>(buffer, offset);
        }
        template<typename T>
        T Read(uint8_t* buffer) {
            T read = *based_pointer<T>(buffer, buffer_position);
            buffer_position += sizeof(T);
            return read;
        }

        void Read(void *dest, uint8_t *src, size_t size) {
            memcpy(dest, src + buffer_position, size);
            buffer_position += size;
        }

        void Seek(size_t new_position) {
            buffer_position = new_position;
        }

        void Advance(size_t amount) {
            buffer_position += amount;
        }

        size_t GetBufferHead() {
            return buffer_position;
        }

        template<typename T>
        T ReadMagic(uint8_t *buffer) const {
            return *based_pointer<T>(buffer, 0);
        }

        std::string ReadString(uint8_t *buffer, uint64_t offset) {
            std::string constructed = "";
            constructed.append(based_pointer<char>(buffer, offset));

            return constructed;
        }

        std::string ReadStringAndAdvance(uint8_t *buffer, uint64_t offset) {
            std::string constructed = "";
            constructed.append(based_pointer<char>(buffer, offset));
            buffer_position += constructed.length();

            return constructed;
        }

        std::string ReadStringAndAdvance(uint8_t *buffer, uint64_t offset, int length) {
            std::string constructed = "";
            constructed.append(based_pointer<char>(buffer, offset), length);
            buffer_position += length;

            return constructed;
        }

        std::string ReadStringWithLength(const uint8_t* buffer, int length) {
            return std::string((const char*)(buffer), length);
        }

        ExeFile* ConvertToExeFile(uint8_t *buffer) {
            return new ExeFile(buffer);
        }

        // Taken directly from GARbro GameRes->ArchiveFormat.cs
        bool IsSaneFileCount(uint32_t file_count) {
            return file_count > 0 && file_count < 0x40000;
        }

        virtual ~ArchiveFormat() = default;

        virtual bool CanHandleFile(uint8_t *buffer, uint64_t size, const std::string &ext) const = 0;
        virtual ArchiveBase* TryOpen(uint8_t *buffer, uint64_t size, std::string file_name) = 0;
        virtual std::string GetTag() const {
            return this->tag;
        };
};
