#include "Utils.h"
#include <util/Logger.h>

#include <ctime>
#include <algorithm>

namespace chrono = std::chrono;

std::string Utils::ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string Utils::GetLastModifiedTime(const std::string& path)
{
    try {
        auto ftime = fs::last_write_time(path);
        auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + chrono::system_clock::now()
        );

        std::time_t tt = chrono::system_clock::to_time_t(sctp);
        std::tm* lt = std::localtime(&tt);

        char buffer[32];
        if (std::strftime(buffer, sizeof(buffer), "%m/%d/%y at %I:%M %p", lt)) {
            return std::string(buffer);
        }
    } catch (const fs::filesystem_error& err) {
        return "N/A";
    }
    return "N/A";
}


std::string Utils::GetFileSize(const fs::path& path)
{
    try {
        if (fs::exists(path) && fs::is_regular_file(path)) {
            uintmax_t size = fs::file_size(path);

            static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
            int unitIndex = 0;
            double decimal_size = static_cast<double>(size);

            while (decimal_size >= 1024.0 && unitIndex < 4) {
                decimal_size /= 1024.0;
                unitIndex++;
            }

            char buffer[64];
            if (unitIndex == 0) {
                std::snprintf(buffer, sizeof(buffer), "%ju %s", static_cast<uintmax_t>(size), units[unitIndex]);
            } else {
                std::snprintf(buffer, sizeof(buffer), "%.2f %s", decimal_size, units[unitIndex]);
            }

            return std::string(buffer);
        }
    } catch (const fs::filesystem_error& err) {
        Logger::error("Error getting file size for %s: %s", path.string().c_str(), err.what());
    }

    return "--";
}

std::string Utils::GetFileSize(u64 size) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double decimal_size = static_cast<double>(size);

    while (decimal_size >= 1024.0 && unitIndex < 4) {
        decimal_size /= 1024.0;
        unitIndex++;
    }

    char buffer[64];
    if (unitIndex == 0) {
        std::snprintf(buffer, sizeof(buffer), "%llu %s", static_cast<unsigned long long>(size), units[unitIndex]);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.2f %s", decimal_size, units[unitIndex]);
    }

    return std::string(buffer);
}
