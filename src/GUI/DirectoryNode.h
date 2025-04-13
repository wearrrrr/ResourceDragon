#pragma once

#include "../common.h"

namespace fs = std::filesystem;

struct DirectoryNode
{
    std::string FullPath;
    std::string FileName;
    std::string FileSize;
    std::string LastModified;
    std::vector<DirectoryNode> Children;
    bool IsDirectory;
};

inline DirectoryNode rootNode;
inline DirectoryNode selectedItem;

DirectoryNode CreateDirectoryNodeTreeFromPath(const fs::path& rootPath);

void ReloadRootNode(DirectoryNode& node);
void ChangeDirectory(DirectoryNode& node, DirectoryNode& rootNode);
void HandleFileClick(DirectoryNode& node);
void DisplayDirectoryNodeRecursive(DirectoryNode& node, DirectoryNode& rootNode);
void DisplayDirectoryNode(DirectoryNode& node, DirectoryNode& rootNode);
void UnloadSelectedFile();