#include "Utils.h"

std::string Utils::GetLastModifiedTime(const std::string& fpath)
{
    try {
        auto ftime = fs::last_write_time(fpath);
        auto sctp = std::chrono::time_point_cast<chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + chrono::system_clock::now()
        );

        std::time_t tt = chrono::system_clock::to_time_t(sctp);
        std::tm* lt = std::localtime(&tt);

        char buffer[32];
        if (std::strftime(buffer, sizeof(buffer), "%m/%d/%y at %I:%M %p", lt)) {
            return std::string(buffer);
        }
        return "N/A";
    } catch (const fs::filesystem_error& e) {
        return "N/A";
    }
}


std::string Utils::GetFileSize(const fs::path& path)
{
    try {
        if (fs::exists(path) && fs::is_regular_file(path)) {
            uintmax_t size = fs::file_size(path);

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
    } catch (const fs::filesystem_error& e) {
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