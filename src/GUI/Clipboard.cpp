#include "Clipboard.h"

const void* ClipboardCopy(void *userdata, const char *mime_type, size_t *size) {
    auto *file = (ClipboardFile*)(userdata);

    if (strcmp(mime_type, "text/uri-list") == 0) {
        *size = file->path.size();
        return file->path.data();
    }
    return nullptr;
}

void ClipboardCleanup(void *userdata) {
    delete (ClipboardFile*)(userdata);
}

void Clipboard::CopyFilePathToClipboard(const std::string &path) {
    ClipboardFile *file = new ClipboardFile();
    file->path = "file://" + path;

    const char *mime_types[] = {"text/uri-list"};

    SDL_SetClipboardData(ClipboardCopy, ClipboardCleanup, file, mime_types, 1);
}
