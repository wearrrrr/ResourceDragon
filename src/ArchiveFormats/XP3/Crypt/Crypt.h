#pragma once

#include <codecvt>
#include "../../../GameRes/Entry.h"

class XP3Crypt {
    public:
        bool HashAfterCrypt = false;
        bool StartupTjsNotEncrypted = false;
        bool ObfuscatedIndex = false;

        virtual std::vector<uint8_t> Decrypt2(Entry *entry, long offset, std::vector<uint8_t> buffer, int pos, int count) = 0;
        virtual uint8_t Decrypt(Entry *entry, long offset, uint8_t value) = 0;
        virtual uint8_t Encrypt(Entry *entry, long offset, uint8_t value) = 0;

        std::u16string ReadName(BinaryReader& header) {
            uint16_t name_size = header.read<uint16_t>();
            if (name_size > 0 && name_size <= 0x100) {
                std::vector<char16_t> buffer(name_size);
                for (int i = 0; i < name_size; ++i) {
                    buffer[i] = header.read<char16_t>();
                }
                return std::u16string(buffer.begin(), buffer.end());
            }
        
            return u"";
        }

        std::string UTF16ToUTF8(const std::u16string& utf16_str) {
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
            return converter.to_bytes(utf16_str);
        }

        std::u16string UTF8ToUTF16(const std::string& utf8_str) {
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
            return converter.from_bytes(utf8_str);
        }

        const char* EntryReadFilter(Entry entry, const char *input, size_t size) {
            // Post processing of the entry data, before being returned by OpenStream
            uint8_t header[5];
            memcpy(header, input, 5);
            uint32_t header_value = 0;
            for (int i = 0; i < 5; ++i) {
                header_value |= (header[i] << (i * 8));
            }
            Logger::log("XP3: Header value: %08X", header_value);


            return input;
        }
};

class NoCrypt : public XP3Crypt {
    public:
        std::vector<uint8_t> Decrypt2(Entry *entry, long offset, std::vector<uint8_t> buffer, int pos, int count) override {
            return buffer;
        }

        uint8_t Decrypt(Entry *entry, long offset, uint8_t value) override {
            return value;
        }
        uint8_t Encrypt(Entry *entry, long offset, uint8_t value) override {
            return value;
        }
};

class HibikiCrypt : XP3Crypt {
    public:
        uint8_t Decrypt(Entry *entry, long offset, uint8_t value) override {
            if (0 != (offset & 4) || offset <= 0x64)
                return (uint8_t)(value ^ (entry->hash >> 5));
            else
                return (uint8_t)(value ^ (entry->hash >> 8));
        }

        std::vector<uint8_t> Decrypt2(Entry *entry, long offset, std::vector<uint8_t> buffer, int pos, int count) override {
            uint8_t key1 = (uint8_t)(entry->hash >> 5);
            uint8_t key2 = (uint8_t)(entry->hash >> 8);
            for (int i = 0; i < count; ++i, ++offset)
            {
                if (0 != (offset & 4) || offset <= 0x64)
                    buffer[pos+i] ^= key1;
                else
                    buffer[pos+i] ^= key2;
            }
            return buffer;
        }

        // no-op
        uint8_t Encrypt(Entry *entry, long offset, uint8_t value) override {
            return value;
        }
};