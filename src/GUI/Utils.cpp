#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../vendored/stb_image/stb_image.h"

std::string Utils::GetLastModifiedTime(const std::string& fpath)
{
    try {
        auto ftime = std::filesystem::last_write_time(fpath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );

        std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
        std::tm* lt = std::localtime(&tt);

        char buffer[32];
        if (std::strftime(buffer, sizeof(buffer), "%m/%d/%y at %I:%M %p", lt)) {
            return std::string(buffer);
        }
        return "N/A";
    } catch (const std::filesystem::filesystem_error& e) {
        return "N/A";
    }
}


std::string Utils::GetFileSize(const std::filesystem::path& path)
{
    try {
        if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
            uintmax_t size = std::filesystem::file_size(path);

            static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
            int unitIndex = 0;
            double readableSize = (double)(size);

            while (readableSize >= 1024.0 && unitIndex < 4) {
                readableSize /= 1024.0;
                unitIndex++;
            }

            std::ostringstream oss;
            if (unitIndex == 0) {
                oss << size << " " << units[unitIndex];
            } else {
                oss << std::fixed << std::setprecision(2) << readableSize << " " << units[unitIndex];
            }

            return oss.str();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        printf("Error getting file size for %s: %s\n", path.string().c_str(), e.what());
    }

    return "0 B";
}

std::string Utils::ShiftJISToUTF8(const std::string& sjisStr) {
    iconv_t conv = iconv_open("UTF-8", "SHIFT-JIS");
    if (conv == (iconv_t)-1) {
        printf("iconv_open failed: %s\n", strerror(errno));
        return "";
    }

    size_t inBytesLeft = sjisStr.size();
    size_t outBytesLeft = inBytesLeft * 2;
    std::vector<char> utf8Str(outBytesLeft);

    char* inBuf = (char*)(sjisStr.data());
    char* outBuf = utf8Str.data();
    
    if (iconv(conv, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft) == (size_t)-1) {
        printf("iconv failed: %s\n", strerror(errno));
        iconv_close(conv);
        return "";
    }

    iconv_close(conv);

    return std::string(utf8Str.data());
}

bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}