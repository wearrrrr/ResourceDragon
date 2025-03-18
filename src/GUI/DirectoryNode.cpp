#include "DirectoryNode.h"
#include <algorithm>

inline std::string ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, const std::filesystem::path& parentPath)
{
    try 
    {
        std::filesystem::directory_iterator directoryIterator(parentPath);

        std::filesystem::path grandParentPath = parentPath.parent_path();
        if (!grandParentPath.empty() && grandParentPath != parentPath)
        {
            DirectoryNode upNode = {
                .FullPath = grandParentPath.string(),
                .FileName = "..",
                .IsDirectory = true
            };
            parentNode.Children.emplace_back(upNode);
        }

        for (const auto& entry : directoryIterator)
        {

            DirectoryNode childNode = {
                .FullPath = entry.path().string(),
                .FileName = entry.path().filename().string(),
                .IsDirectory = entry.is_directory()
            };

			parentNode.Children.push_back(childNode);
        }

        std::sort(parentNode.Children.begin(), parentNode.Children.end(), 
            [](const DirectoryNode& a, const DirectoryNode& b) 
            {
                if (a.FileName == "..") return true;
                if (b.FileName == "..") return false;
                if (a.IsDirectory != b.IsDirectory) return a.IsDirectory > b.IsDirectory;
                return ToLower(a.FileName) < ToLower(b.FileName);
            }
        );
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        printf("Error accessing directory: %s\n", e.what());
    }
}


DirectoryNode CreateDirectoryNodeTreeFromPath(const std::filesystem::path& rootPath)
{

    DirectoryNode rootNode = {
        .FullPath = rootPath.string(),
        .FileName = rootPath.string(),
        .IsDirectory = std::filesystem::is_directory(rootPath)
    };

    if (rootNode.IsDirectory)
    {
        RecursivelyAddDirectoryNodes(rootNode, rootPath);
    }

    return rootNode;
}
