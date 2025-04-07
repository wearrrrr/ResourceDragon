#include "Utils.h"

std::string Utils::ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string Utils::GetLastModifiedTime(const std::string& fpath)
{
    namespace chrono = std::chrono;
    try {
        auto ftime = fs::last_write_time(fpath);
        auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + chrono::system_clock::now()
        );

        std::time_t tt = chrono::system_clock::to_time_t(sctp);
        std::tm* lt = std::localtime(&tt);

        char buffer[32];
        if (std::strftime(buffer, sizeof(buffer), "%m/%d/%y at %I:%M %p", lt)) {
            return std::string(buffer);
        }
    } catch (const fs::filesystem_error& e) {
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
            double decimal_size = (double)(size);

            while (decimal_size >= 1024.0 && unitIndex < 4) {
                decimal_size /= 1024.0;
                unitIndex++;
            }

            std::ostringstream oss;
            if (unitIndex == 0) {
                oss << size << " " << units[unitIndex];
            } else {
                oss << std::fixed << std::setprecision(2) << decimal_size << " " << units[unitIndex];
            }

            return oss.str();
        }
    } catch (const fs::filesystem_error& e) {
        Logger::error("Error getting file size for %s: %s\n", path.string().c_str(), e.what());
    }

    return "0 B";
}