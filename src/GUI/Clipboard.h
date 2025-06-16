#include <SDL3/SDL.h>
#include <string>
#include <util/int.h>

struct ClipboardFile {
    std::string path;
    std::string mime_type;
    const void *buffer;
    size_t size;
};

namespace Clipboard {
    void CopyBufferToClipboard(u8 *buffer, size_t size, std::string file_name);
    void CopyFilePathToClipboard(const std::string &path);
};
