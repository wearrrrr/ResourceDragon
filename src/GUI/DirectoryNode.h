#include <string>
#include <vector>
#include <filesystem>

struct DirectoryNode
{
    std::string FullPath;
    std::string FileName;
    std::vector<DirectoryNode> Children;
    bool IsDirectory;
    bool IsLoaded = false;
    
    DirectoryNode(std::string fileName, std::string fullPath, bool isDirectory)
        : FullPath(std::move(fullPath)), FileName(std::move(fileName)), IsDirectory(isDirectory) {}
    
    DirectoryNode() = default;
};

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, const std::filesystem::directory_iterator& directoryIterator);
DirectoryNode CreateDirectoryNodeTreeFromPath(const std::filesystem::path& rootPath);
