#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "../common.h"
#include "Image.h"

namespace fs = std::filesystem;

struct DirectoryNode
{
    std::string FullPath;
    std::string FileName;
    std::string LastModified;
    std::vector<DirectoryNode> Children;
    bool IsDirectory;
};

extern DirectoryNode rootNode;

DirectoryNode CreateDirectoryNodeTreeFromPath(const fs::path& rootPath);

void ReloadRootNode(DirectoryNode& node);
void ChangeDirectory(DirectoryNode& node, DirectoryNode& rootNode);
void HandleFileClick(DirectoryNode& node);
void DisplayDirectoryNodeRecursive(DirectoryNode& node, DirectoryNode& rootNode);
void DisplayDirectoryNode(DirectoryNode& node, DirectoryNode& rootNode, bool isRoot);