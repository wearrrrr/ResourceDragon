#include <filesystem>
#include <string>
#include <ctime>

class Utils {
    public:
        static std::string GetLastModifiedTime(const std::string& fpath);
        static std::string GetFileSize(const std::filesystem::path& path);
};