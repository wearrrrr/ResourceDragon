#include <string>
#include <vector>
#include <filesystem>

struct DirectoryNode
{
    std::string FullPath;
    std::string FileName;
    std::vector<DirectoryNode> Children;
    bool IsDirectory;
};

DirectoryNode CreateDirectoryNodeTreeFromPath(const std::filesystem::path& rootPath);
