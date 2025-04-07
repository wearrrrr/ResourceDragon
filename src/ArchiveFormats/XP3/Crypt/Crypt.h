#pragma once

#include "../../../GameRes/Entry.h"

class XP3Crypt {
    public:
        bool HashAfterCrypt = false;
        bool StartupTjsNotEncrypted = false;
        bool ObfuscatedIndex = false;

        virtual std::vector<uint8_t> Decrypt(Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) = 0;
        virtual uint8_t Decrypt(Entry *entry, uint64_t offset, uint8_t value) = 0;
        virtual uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) = 0;

        virtual ~XP3Crypt() = default;

        virtual std::string GetCryptName() {
            return "Default (BAD!!)";
        }

        std::u16string ReadName(BinaryReader& header) {
            int16_t name_size = header.read<int16_t>();
            if (name_size > 0 && name_size <= 0x100) {
                std::vector<int16_t> buffer(name_size);
                for (int i = 0; i < name_size; ++i) {
                    buffer[i] = header.read<int16_t>();
                }
                return std::u16string(buffer.begin(), buffer.end());
            }
        
            return u"";
        }

        const char* EntryReadFilter(Entry entry, const char *input, size_t size) {
            // Post processing of the entry data, before being returned by OpenStream
            return input;
        }
};


class NoCrypt : public XP3Crypt {
    public:
        std::vector<uint8_t> Decrypt(Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) override {
            Logger::log("NoCrypt: Decrypting %d bytes at offset %d", count, offset);
            return buffer;
        }

        uint8_t Decrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            return value;
        }
        uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "NoCrypt";
        }
};

class HibikiCrypt : XP3Crypt {
    public:
        uint8_t Decrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            if (0 != (offset & 4) || offset <= 0x64)
                return (uint8_t)(value ^ (entry->hash >> 5));
            else
                return (uint8_t)(value ^ (entry->hash >> 8));
        }

        std::vector<uint8_t> Decrypt(Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) override {
            uint8_t key1 = (uint8_t)(entry->hash >> 5);
            uint8_t key2 = (uint8_t)(entry->hash >> 8);
            for (int i = 0; i < count; i++) {
                offset++;
                if ((offset & 4) != 0 || offset <= 0x64)
                    buffer[pos+i] ^= key1;
                else
                    buffer[pos+i] ^= key2;
            }
            return buffer;
        }

        // no-op
        uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "Hibiki";
        }
};

class AkabeiCrypt : public XP3Crypt {
    private:
        uint32_t m_seed;
    
    public:
        AkabeiCrypt() : m_seed(0) {}
        AkabeiCrypt(uint32_t seed) : m_seed(seed) {}
    
        std::string ToString() const {
            std::ostringstream oss;
            oss << "(0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << m_seed << ")";
            return oss.str();
        }
    
        uint8_t Decrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            int key_pos = (int)(offset) & 0x1F;
            auto key = GetKey(entry->hash);
            return value ^ key[key_pos];
        }
    
        std::vector<uint8_t> Decrypt(Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) override {
            auto key = GetKey(entry->hash);
            int key_pos = (int)(offset);
            for (int i = 0; i < count; ++i) {
                buffer[pos + i] ^= key[key_pos++ & 0x1F];
            }
            return buffer;
        }
    
        uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "Akabei";
        }
    
    private:
        std::vector<uint8_t> GetKey(uint32_t hash) const {
            std::vector<uint8_t> key;
            hash = (hash ^ m_seed) & 0x7FFFFFFF;
            hash = (hash << 31) | hash;
            for (int i = 0; i < 0x20; ++i) {
                key.push_back((uint8_t)hash);
                hash = ((hash & 0xFFFFFFFE) << 23) | (hash >> 8);
            }
            return key;
        }
};