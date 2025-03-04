#include "Utils.h"

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