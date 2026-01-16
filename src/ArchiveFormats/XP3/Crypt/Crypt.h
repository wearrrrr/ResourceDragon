#pragma once

#include "SDK/util/Logger.hpp"
#include <iomanip>
#include <sstream>

#include <Entry.h>
#include <BinaryReader.h>

class XP3Crypt {
public:
        bool HashAfterCrypt = false;
        bool StartupTjsNotEncrypted = false;
        bool ObfuscatedIndex = false;

        virtual std::vector<u8> Decrypt(const Entry *entry, u64 offset, std::vector<u8> buffer, int pos, int count) = 0;
        virtual u8 Encrypt(Entry *entry, u64 offset, u8 value) = 0;

        virtual ~XP3Crypt() = default;

        virtual std::string GetCryptName() {
            return "Default";
        }

        std::u16string ReadName(BinaryReader &header) {
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
        std::vector<u8> Decrypt(const Entry *entry, u64 offset, std::vector<u8> buffer, int pos, int count) override {
            return buffer;
        }
        u8 Encrypt(Entry *entry, u64 offset, u8 value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "NoCrypt";
        }
};

class HibikiCrypt : public XP3Crypt {
    public:
        std::vector<u8> Decrypt(const Entry *entry, u64 offset, std::vector<u8> buffer, int pos, int count) override {
            u8 key1 = (u8)(entry->hash >> 5);
            u8 key2 = (u8)(entry->hash >> 8);
            for (int i = 0; i < count; ++i, ++offset) {
                if ((offset & 4) != 0 || offset <= 0x64)
                    buffer[pos+i] ^= key1;
                else
                    buffer[pos+i] ^= key2;
            }
            return buffer;
        }

        // no-op
        u8 Encrypt(Entry *entry, u64 offset, u8 value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "HibikiCrypt";
        }
};

class AkabeiCrypt : public XP3Crypt {
    private:
        u32 m_seed;

    public:
        AkabeiCrypt() {
            m_seed = 0xE5BDEC8A;
        }
        AkabeiCrypt(u32 seed) : m_seed(seed) {}

        std::vector<u8> Decrypt(const Entry *entry, u64 offset, std::vector<u8> buffer, int pos, int count) override {
            std::vector<u8> out = buffer;

            auto key = GetKey(entry->hash);
            int key_pos = (int)(offset);
            for (int i = 0; i < count; ++i) {
                out[pos+i] ^= key[key_pos++ & 0x1F];
            }
            return out;
        }

        u8 Encrypt(Entry *entry, u64 offset, u8 value) override {
            return value;
        }

        std::string GetCryptName() override {
            return "AkabeiCrypt";
        }

    private:
        std::vector<u8> GetKey(u32 hash) const {
            std::vector<u8> key;
            hash = (hash ^ m_seed) & 0x7FFFFFFF;
            hash = (hash << 31) | hash;
            for (int i = 0; i < 0x20; ++i) {
                key.push_back((u8)hash);
                hash = (hash & 0xFFFFFFFE) << 23 | hash >> 8;
            }
            return key;
        }
};

class SmileCrypt : XP3Crypt {
    private:
        u32 m_key_xor;
        u8 m_first_xor;
        u8 m_zero_xor;

    public:
    SmileCrypt (u32 key_xor, u8 first_xor, u8 zero_xor) {
        m_key_xor = key_xor;
        m_first_xor = first_xor;
        m_zero_xor = zero_xor;
    }

    std::vector<u8> Decrypt(const Entry *entry, u64 offset, std::vector<u8> buffer, int pos, int count) override {
        u32 hash = entry->hash ^ m_key_xor;
        u8 key = (u8)(hash ^ (hash >> 8) ^ (hash >> 16) ^ (hash >> 24));
        if (key == 0) {
            key = m_zero_xor;
        }
        if (offset == 0 && count > 0) {
            if ((hash & 0xFF) == 0) {
                hash = m_first_xor;
            }
            buffer[pos] ^= (u8)hash;
        }
        for (int i = 0; i < count; i++) {
            buffer[pos+1] ^= key;
        }
        return buffer;
    }

    u8 Encrypt(Entry *entry, u64 offset, u8 value) override {
        return value;
    }

    std::string GetCryptName() override {
        return "SmileCrypt";
    }
};
