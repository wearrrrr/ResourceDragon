#include <SDL3/SDL.h>
#include <string>
#include <fstream>
#include <filesystem>
#include "../util/Logger.h"

namespace fs = std::filesystem;

struct ClipboardFile {
    std::string path;
    std::string mime_type;
    const void *buffer;
    size_t size;
};

namespace Clipboard {
    void CopyBufferToClipboard(unsigned char *buffer, size_t size, std::string file_name);
    void CopyFilePathToClipboard(const std::string &path);
};