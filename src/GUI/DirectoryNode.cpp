#include "DirectoryNode.h"
#include <iostream>
#include <algorithm>

// Helper function for case-insensitive sorting
inline std::string ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, const std::filesystem::path& parentPath)
{
    parentNode.IsLoaded = true; // Mark directory as loaded

    try 
    {
        std::filesystem::directory_iterator directoryIterator(parentPath);

        // Add ".." (parent directory) if applicable
        std::filesystem::path grandParentPath = parentPath.parent_path();
        if (!grandParentPath.empty() && grandParentPath != parentPath)
        {
            parentNode.Children.emplace_back("..", grandParentPath.string(), true);
            std::cout << "Added parent: " << grandParentPath.string() << std::endl;
        }

        // Add children
        for (const auto& entry : directoryIterator)
        {
			DirectoryNode childNode;
			childNode.FullPath = entry.path().string();
			childNode.FileName = entry.path().filename().string();
			childNode.IsDirectory = entry.is_directory();

			parentNode.Children.push_back(std::move(childNode));
        }

        // Sort: ".." first, then directories, then files
        std::sort(parentNode.Children.begin(), parentNode.Children.end(), 
            [](const DirectoryNode& a, const DirectoryNode& b) 
            {
                if (a.FileName == "..") return true;
                if (b.FileName == "..") return false;
                if (a.IsDirectory != b.IsDirectory) return a.IsDirectory > b.IsDirectory;
                return ToLower(a.FileName) < ToLower(b.FileName);
            });

    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Error accessing directory: " << e.what() << std::endl;
    }
}


DirectoryNode CreateDirectoryNodeTreeFromPath(const std::filesystem::path& rootPath)
{
    DirectoryNode rootNode(rootPath.string(), rootPath.string(), std::filesystem::is_directory(rootPath));
    
    if (rootNode.IsDirectory)
    {
        RecursivelyAddDirectoryNodes(rootNode, rootPath);
    }

    return rootNode;
}
