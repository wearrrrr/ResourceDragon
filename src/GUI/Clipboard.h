#include <SDL3/SDL.h>
#include <string>

struct ClipboardFile {
    std::string path;
    std::string mime_type;
};

namespace Clipboard {
    void CopyFilePathToClipboard(const std::string &path);
};