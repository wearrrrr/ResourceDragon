#pragma once

#include <iomanip>
#include <sstream>

#include <Entry.h>
#include "../../../BinaryReader.h"

class XP3Crypt {
public:
        bool HashAfterCrypt = false;
        bool StartupTjsNotEncrypted = false;
        bool ObfuscatedIndex = false;

        virtual std::vector<uint8_t> Decrypt(const Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) = 0;
        virtual uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) = 0;

        virtual ~XP3Crypt() = default;

        virtual std::string GetCryptName() {
            return "Default";
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
};


class NoCrypt : public XP3Crypt {
    public:
        std::vector<uint8_t> Decrypt(const Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) override {
            return buffer;
        }
        uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "NoCrypt";
        }
};

class HibikiCrypt : public XP3Crypt {
    public:
        std::vector<uint8_t> Decrypt(const Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) override {
            uint8_t key1 = (uint8_t)(entry->hash >> 5);
            uint8_t key2 = (uint8_t)(entry->hash >> 8);
            for (int i = 0; i < count; ++i, ++offset) {
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
            return "HibikiCrypt";
        }
};

class AkabeiCrypt : public XP3Crypt {
    private:
        uint32_t m_seed;

    public:
        AkabeiCrypt() {
            m_seed = 0xE5BDEC8A;
        }
        AkabeiCrypt(uint32_t seed) : m_seed(seed) {}

        std::string ToString() const {
            std::ostringstream oss;
            oss << "(0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << m_seed << ")";
            return oss.str();
        }

        std::vector<uint8_t> Decrypt(const Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) override {
            std::vector<uint8_t> out = buffer;

            auto key = GetKey(entry->hash);
            int key_pos = (int)(offset);
            for (int i = 0; i < count; ++i) {
                out[pos+i] ^= key[key_pos++ & 0x1F];
            }
            return out;
        }

        uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "AkabeiCrypt";
        }

    private:
        std::vector<uint8_t> GetKey(uint32_t hash) const {
            std::vector<uint8_t> key;
            hash = (hash ^ m_seed) & 0x7FFFFFFF;
            hash = (hash << 31) | hash;
            for (int i = 0; i < 0x20; ++i) {
                key.push_back((uint8_t)hash);
                hash = (hash & 0xFFFFFFFE) << 23 | hash >> 8;
            }
            return key;
        }
};

class SmileCrypt : XP3Crypt {
    private:
        uint32_t m_key_xor;
        uint8_t m_first_xor;
        uint8_t m_zero_xor;

    public:
    SmileCrypt (uint32_t key_xor, uint8_t first_xor, uint8_t zero_xor) {
        m_key_xor = key_xor;
        m_first_xor = first_xor;
        m_zero_xor = zero_xor;
    }

    std::vector<uint8_t> Decrypt(const Entry *entry, uint64_t offset, std::vector<uint8_t> buffer, int pos, int count) override {
        uint32_t hash = entry->hash ^ m_key_xor;
        uint8_t key = (uint8_t)(hash ^ (hash >> 8) ^ (hash >> 16) ^ (hash >> 24));
        if (key == 0) {
            key = m_zero_xor;
        }
        if (offset == 0 && count > 0) {
            if ((hash & 0xFF) == 0) {
                hash = m_first_xor;
            }
            buffer[pos] ^= (uint8_t)hash;
        }
        for (int i = 0; i < count; i++) {
            buffer[pos+1] ^= key;
        }
        return buffer;
    }

    uint8_t Encrypt(Entry *entry, uint64_t offset, uint8_t value) override {
        return value;
    }

    std::string GetCryptName() override {
        return "SmileCrypt";
    }
};
