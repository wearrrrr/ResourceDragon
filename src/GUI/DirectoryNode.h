#include <string>
#include <vector>
#include <filesystem>

struct DirectoryNode
{
    std::string FullPath;
    std::string FileName;
    std::vector<DirectoryNode> Children;
    bool IsDirectory;
    
    DirectoryNode(std::string fileName, std::string fullPath, bool isDirectory)
        : FullPath(fullPath), FileName(fileName), IsDirectory(isDirectory) {}
    
    DirectoryNode() = default;
};

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, const std::filesystem::directory_iterator& directoryIterator);
DirectoryNode CreateDirectoryNodeTreeFromPath(const std::filesystem::path& rootPath);
