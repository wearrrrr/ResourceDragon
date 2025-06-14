#include <Clipboard.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

const void* ClipboardCopy(void *userdata, const char *mime_type, size_t *size) {
    auto *file = (ClipboardFile*)(userdata);

    if (strcmp(mime_type, "text/uri-list") == 0) {
        *size = file->path.size();
        return file->path.data();
    }

    if (strcmp(mime_type, file->mime_type.c_str()) == 0) {
        *size = file->size;
        return file->buffer;
    }

    return nullptr;
}

void ClipboardCleanup(void *userdata) {
    delete (ClipboardFile*)(userdata);
}

void Clipboard::CopyBufferToClipboard(uint8_t *buffer, size_t size, std::string file_name) {
    std::string tempPath = "/tmp/rd/";
    std::string filename = tempPath + file_name;

    fs::create_directories(tempPath);
    std::ofstream outFile(filename, std::ios::binary);
    outFile.write((const char*)buffer, size);
    outFile.close();

    CopyFilePathToClipboard(filename);

    return;
}

void Clipboard::CopyFilePathToClipboard(const std::string &path)
{
    ClipboardFile *file = new ClipboardFile();
    file->path = "file://" + path;

    const char *mime_types[] = {"text/uri-list"};

    SDL_SetClipboardData(ClipboardCopy, ClipboardCleanup, file, mime_types, 1);
}
