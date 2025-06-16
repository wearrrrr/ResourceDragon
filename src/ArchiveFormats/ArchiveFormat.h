#pragma once

#include "ExeFile.h"
#include "Entry.h"
#include <cstring>
#include <unordered_map>
#include <algorithm>
#include <util/Logger.h>
#include <util/int.h>
#include "zero_templates.h"

template <typename T = std::string>
bool VectorHas(std::vector<T> vec, T item) {
    return std::find(vec.begin(), vec.end(), item) != vec.end();
}

class ArchiveBase {
    public:
        virtual const char* OpenStream(const Entry *entry, u8 *buffer) = 0;
        virtual std::unordered_map<std::string, Entry*> GetEntries() = 0;
        virtual ~ArchiveBase() = default;
};

class ArchiveFormat {
    public:
        std::string tag = "?????";
        std::string description = "????? Resource Archive";
        u32 sig = 0x00000000;

        size_t buffer_position = 0;

        template<typename T>
        T Read(u8 *buffer, usize offset) const {
            return *based_pointer<T>(buffer, offset);
        }
        template<typename T>
        T Read(u8 *buffer) {
            T read = *based_pointer<T>(buffer, buffer_position);
            buffer_position += sizeof(T);
            return read;
        }

        void Read(void *dest, u8 *src, usize size) {
            memcpy(dest, src + buffer_position, size);
            buffer_position += size;
        }

        void Seek(usize new_position) {
            buffer_position = new_position;
        }

        void Advance(usize amount) {
            buffer_position += amount;
        }

        size_t GetBufferHead() {
            return buffer_position;
        }

        template<typename T>
        T ReadMagic(u8 *buffer) const {
            return *based_pointer<T>(buffer, 0);
        }

        std::string ReadString(u8 *buffer, u64 offset) {
            std::string constructed = "";
            constructed.append(based_pointer<char>(buffer, offset));

            return constructed;
        }

        std::string ReadStringAndAdvance(u8 *buffer, u64 offset) {
            std::string constructed = "";
            constructed.append(based_pointer<char>(buffer, offset));
            buffer_position += constructed.length();

            return constructed;
        }

        std::string ReadStringAndAdvance(u8 *buffer, u64 offset, usize length) {
            std::string constructed = "";
            constructed.append(based_pointer<char>(buffer, offset), length);
            buffer_position += length;

            return constructed;
        }

        std::string ReadStringWithLength(const u8* buffer, usize length) {
            return std::string((const char*)(buffer), length);
        }

        ExeFile* ConvertToExeFile(u8 *buffer) {
            return new ExeFile(buffer);
        }

        // Taken directly from GARbro GameRes->ArchiveFormat.cs
        bool IsSaneFileCount(u32 file_count) {
            return file_count > 0 && file_count < 0x40000;
        }

        virtual ~ArchiveFormat() = default;

        virtual bool CanHandleFile(u8 *buffer, u64 size, const std::string &ext) const = 0;
        virtual ArchiveBase* TryOpen(u8 *buffer, u64 size, std::string file_name) = 0;
        virtual std::string GetTag() const {
            return this->tag;
        };
};
