#include "xp3.h"

// TODO! This will need a large refactor, especially since I am planning on greatly changing how TryOpen works in the future.
// For now, this will hardcode returning 

static ICrypt *ALG_DEFAULT = (ICrypt*)new HibikiCrypt();

ICrypt *GuessCryptAlgorithm(unsigned char *buffer, uint32_t size, std::string file_name) {
    return nullptr;
};

ICrypt *QueryCryptAlgorithm(unsigned char *buffer, uint32_t size, std::string file_name) {
    ICrypt *alg = GuessCryptAlgorithm(buffer, size, file_name);

    if (alg == nullptr) {
        Logger::error("XP3: Failed to guess crypt algorithm! Returning default...");
        return ALG_DEFAULT;
    }
    return alg;
}

class FilenameMap {
    private:
        std::unordered_map<uint32_t, std::string> m_hash_map;
        std::unordered_map<std::string, std::string> m_md5_map;
    
        std::string GetMd5Hash(const std::string& text) {
            unsigned char hash[EVP_MAX_MD_SIZE];
            unsigned int hash_len;
            
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
            EVP_DigestUpdate(ctx, text.c_str(), text.size());
            EVP_DigestFinal_ex(ctx, hash, &hash_len);
            EVP_MD_CTX_free(ctx);
    
            std::ostringstream oss;
            for (unsigned int i = 0; i < hash_len; ++i) {
                oss << std::hex << std::setw(2) << std::setfill('0') << (int)(hash[i]);
            }
            return oss.str();
        }
    
    public:
        size_t Count() const { return m_md5_map.size(); }
    
        void Add(uint32_t hash, const std::string& filename) {
            if (m_hash_map.find(hash) == m_hash_map.end()) {
                m_hash_map[hash] = filename;
            }
            m_md5_map[GetMd5Hash(filename)] = filename;
        }
    
        void AddShortcut(const std::string& shortcut, const std::string& filename) {
            m_md5_map[shortcut] = filename;
        }
    
        std::string Get(uint32_t hash, const std::string& md5) const {
            auto md5_it = m_md5_map.find(md5);
            if (md5_it != m_md5_map.end()) {
                return md5_it->second;
            }
            auto hash_it = m_hash_map.find(hash);
            if (hash_it != m_hash_map.end()) {
                return hash_it->second;
            }
            return md5;
        }
    };

ArchiveBase *XP3Format::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name)
{
    uint64_t base_offset = 0;
    // For now, let's ignore the fact that this can be an EXE file, I am lazy.

    if (!CanHandleFile(buffer, size, "")) {
        Logger::error("XP3: Invalid file signature!");
        return nullptr;
    }
    uint64_t dir_offset = *based_pointer<uint64_t>(buffer, base_offset + 0x0B);
    if (dir_offset < 0x13 || dir_offset >= size) {
        Logger::error("XP3: Directory offset is invalid!");
        return nullptr;
    }
    if (*based_pointer<uint32_t>(buffer, dir_offset) == 0x80) {
        dir_offset = base_offset + *based_pointer<uint32_t>(buffer, dir_offset + 0x9);
        if (dir_offset < 0x13) {
            Logger::error("XP3: Directory offset is invalid!");
            return nullptr;
        }
    }
    int header_type = *based_pointer<uint8_t>(buffer, dir_offset);
    if (header_type != XP3_HEADER_UNPACKED && header_type != XP3_HEADER_PACKED) {
        Logger::error("XP3: Header type is invalid!");
        return nullptr;
    }

    Logger::log("XP3 Header type: %s", header_type == XP3_HEADER_UNPACKED ? "Unpacked" : "Packed");

    std::vector<uint8_t> header_stream;
    if (header_type == XP3_HEADER_UNPACKED) {
        int64_t header_size = *based_pointer<int64_t>(buffer, dir_offset + 0x1);
        Logger::log("XP3 Header size: %d", header_size);
        if (header_size > std::numeric_limits<int64_t>::max()) {
            Logger::error("XP3: Header size is invalid!");
            return nullptr;
        }
        header_stream.resize(header_size);
        memcpy(header_stream.data(), buffer + dir_offset + 0x9, header_size);
        Logger::log("%x %x %x %x", header_stream[0], header_stream[1], header_stream[2], header_stream[3]);
    } else {
        uint64_t packed_size = *based_pointer<uint64_t>(buffer, dir_offset + 0x1);
        if (packed_size > std::numeric_limits<uint64_t>::max()) {
            Logger::error("XP3: Packed size is invalid!");
            return nullptr;
        }
        uint64_t header_size = *based_pointer<uint64_t>(buffer, dir_offset + 0x9);
        const uint8_t* compressed_data = buffer + dir_offset + 0x11;
    
        header_stream.resize(header_size);
    
        // Convert `header_size` to `uLongf`
        uLongf decompressed_size = (uLongf)(header_size);
        uLongf compressed_size = (uLongf)(packed_size);
    
        if (uncompress(header_stream.data(), &decompressed_size, compressed_data, compressed_size) != Z_OK) {
            Logger::error("XP3: Failed to decompress header!");
            return nullptr;
        }

        if (decompressed_size != header_size) {
            Logger::error("XP3: Decompressed size does not match header size!");
            return nullptr;
        }
        Logger::log("XP3 Header size after decompression: %lu", decompressed_size);
    }

    ICrypt *crypt = QueryCryptAlgorithm(buffer, size, file_name);

    std::vector<Entry> entries;
    dir_offset = 0;

    size_t position = 0;

    BinaryReader header(header_stream);
    FilenameMap filename_map;

    while (header.position < header.data.size() && header.peek().has_value()) {
        uint32_t entry_signature = header.read<uint32_t>();
        int64_t entry_size = header.read<int64_t>();
        if (entry_size < 0) return nullptr;
        dir_offset += 12 + entry_size;

        if (PackUInt32('F', 'i', 'l', 'e') == entry_signature) {
            XP3Entry entry;
            while (entry_size > 0) {
                uint32_t section = header.read<uint32_t>();
                int64_t section_size = header.read<int64_t>();
                entry_size -= 12;
                if (section_size > entry_size)
                {
                    // allow "info" sections with wrong size
                    if (section != 0x6f666e69)
                        break;
                    section_size = entry_size;
                }
                entry_size -= section_size;
                int64_t next_section_pos = header.position + section_size;
                printf("%ld\n", next_section_pos);

                switch (section) {
                    // "info"
                    case 0x6f666e69: {
                        if (entry.size != 0 || !entry.name.empty()) {
                            header.position = dir_offset;
                        }
                        entry.m_isEncrypted = header.read<uint32_t>() != 0;
                        int64_t file_size = header.read<int64_t>();
                        int64_t packed_size = header.read<int64_t>();

                        uint64_t uint_max = std::numeric_limits<uint64_t>::max();

                        if (file_size >= uint_max || packed_size > uint_max || packed_size > size) {
                            header.position = dir_offset;
                        }
                        entry.IsPacked = file_size != packed_size;
                        entry.size = (uint32_t)packed_size;
                        entry.UnpackedSize = (uint32_t)file_size;

                        // TODO: change this to use inferred crypt
                        if (entry.m_isEncrypted) {
                            entry.m_crypt = ALG_DEFAULT;
                        }

                        std::string name = entry.m_crypt->ReadName(header);
                        // todo: temporary test
                        if (name == "RD_INVALID_FILE_NAME") {
                            header.position = dir_offset;
                        }

                        if (filename_map.Count() > 0) name = filename_map.Get(entry.m_hash, name);
                        if (name.size() > 0x100)
                        {
                            header.position = dir_offset;
                        }
                        entry.name = name;
                    }
                
                    // "segm"
                    case 0x6d676573: {
                        int segment_count = (int)(section_size / 0x1c);
                        if (segment_count > 0) {
                            for (int i = 0; i < segment_count; ++i) {
                                bool compressed  = 0 != header.read<int32_t>();
                                long segment_offset = base_offset + header.read<int64_t>();
                                long segment_size   = header.read<int64_t>();
                                long segment_packed_size = header.read<int64_t>();
                                if (segment_offset > size || segment_packed_size > size) {
                                    header.position = dir_offset;
                                }
                                XP3Segment segment = {
                                    .IsCompressed = compressed,
                                    .Offset       = segment_offset,
                                    .Size         = (uint32_t)segment_size,
                                    .PackedSize   = (uint32_t)segment_packed_size
                                };
                                entry.m_segments.push_back(segment);
                            }
                            entry.offset = entry.m_segments.begin()->Offset;
                        }
                        break;
                    }
                    
                    
                    // "adlr"
                    case 0x726c6461: {
                        if (4 == section_size) {
                            uint32_t hash = header.read<uint32_t>();
                            entry.m_hash = hash;
                        }
                        break;
                    }
                    // unknown section
                    default:
                        break;
                }
                header.position = next_section_pos;
            }
            if (!entry.name.empty() && entry.m_segments.size() > 0) {
                // if (entry.m_crypt.ObfuscatedIndex)
                // {
                //     DeobfuscateEntry (entry);
                // }
                entries.push_back(entry);
            }
        }
        else if (entry_size > 7)
        {
            // 0x6E666E68 == entry_signature    // "hnfn"
            // 0x6C696D73 == entry_signature    // "smil"
            // 0x46696C65 == entry_signature    // "eliF"
            // 0x757A7559 == entry_signature    // "Yuzu"
            uint hash = header.read<uint32_t>();
            int name_size = header.read<int16_t>();
            if (name_size > 0)
            {
                entry_size -= 6;
                if (name_size * 2 <= entry_size)
                {
                    std::string filename = header.ReadChars(name_size);
                    filename_map.Add(hash, filename);
                }
            }
        }
    }
    Logger::log("Finished reading %d bytes", header_stream.size());
    Logger::log("%d", entries.size());
    Logger::log("0x%x", entries.at(0).size);

    return nullptr;
}

const char *XP3Archive::OpenStream(const Entry &entry, unsigned char *buffer)
{
    return nullptr;
}

std::vector<Entry> XP3Archive::GetEntries()
{
    return std::vector<Entry>();
}