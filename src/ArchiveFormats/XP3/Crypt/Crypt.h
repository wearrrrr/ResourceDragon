#pragma once

#include "../xp3entry.h"

class ICrypt {
    public:
        bool HashAfterCrypt = false;
        bool StartupTjsNotEncrypted = false;
        bool ObfuscatedIndex = false;

        virtual uint8_t Decrypt(XP3Entry *entry, long offset, uint8_t value) = 0;
        virtual uint8_t Encrypt(XP3Entry *entry, long offset, uint8_t value) = 0;

        std::string ReadName(BinaryReader& header) {
            int16_t name_size = header.read<int16_t>();  // Read a 16-bit integer
        
            if (name_size > 0 && name_size <= 0x100) {
                return header.ReadChars(name_size);
            }
        
            return "";
        }
};

class NoCrypt : public ICrypt {
    public:
        uint8_t Decrypt(XP3Entry *entry, long offset, uint8_t value) override {
            return value;
        }
        uint8_t Encrypt(XP3Entry *entry, long offset, uint8_t value) override {
            return value;
        }
};

class HibikiCrypt : ICrypt {
    public:
        uint8_t Decrypt(XP3Entry *entry, long offset, uint8_t value) override {
            if (0 != (offset & 4) || offset <= 0x64)
                return (uint8_t)(value ^ (entry->m_hash >> 5));
            else
                return (uint8_t)(value ^ (entry->m_hash >> 8));
        }

        void Decrypt(XP3Entry *entry, long offset, std::vector<uint8_t> buffer, int pos, int count) {
            uint8_t key1 = (uint8_t)(entry->m_hash >> 5);
            uint8_t key2 = (uint8_t)(entry->m_hash >> 8);
            for (int i = 0; i < count; ++i, ++offset)
            {
                if (0 != (offset & 4) || offset <= 0x64)
                    buffer[pos+i] ^= key1;
                else
                    buffer[pos+i] ^= key2;
            }
        }

        // no-op
        uint8_t Encrypt(XP3Entry *entry, long offset, uint8_t value) override {
            return value;
        }
};