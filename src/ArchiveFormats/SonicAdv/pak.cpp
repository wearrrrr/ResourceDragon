#include "pak.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

ArchiveBase *SAPakFormat::TryOpen(unsigned char *buffer, uint32_t size, std::string file_name) {
    uint32_t file_count = Read<uint32_t>(buffer, 0x39);

    Logger::log("%s: %d files", file_name.c_str(), file_count);

    std::vector<std::string> long_paths(file_count);
    std::vector<std::string> file_names(file_count);
    std::vector<uint32_t> file_lengths(file_count);

    Seek(0x3D);
    
    for (uint32_t i = 0; i < file_count; i++) {
        uint32_t name_len = Read<uint32_t>(buffer);
        long_paths[i] = ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        name_len = Read<uint32_t>(buffer);
        file_names[i] = ReadStringAndAdvance(buffer, GetBufferHead(), name_len);
        file_lengths[i] = Read<uint32_t>(buffer);

        Logger::log("%s: %s", file_name.c_str(), long_paths[i].c_str());
        Logger::log("%s: %s", file_name.c_str(), file_names[i].c_str());
        Logger::log("%s: %ld", file_name.c_str(), file_lengths[i]);

        Advance(0x4);
    }

    for (uint32_t i = 0; i < file_count; i++) {
        std::vector<uint8_t> entry_data;
        entry_data.resize(file_lengths[i]);
        Read(entry_data.data(), buffer, file_lengths[i]);
        // std::replace(file_names[i].begin(), file_names[i].end(), '\\', '/');
        // std::string output_path = "output/" + file_names[i];
        // fs::create_directories("output/" + fs::path(file_names[i]).parent_path().string());
        // std::ofstream outFile(output_path, std::ios::binary);
        // outFile.write((const char*)entry_data.data(), file_lengths[i]);
        // outFile.close();
        // Logger::log("%s: %s", file_name.c_str(), output_path.c_str());
    }

    return nullptr;
};