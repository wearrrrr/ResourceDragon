#pragma once

#include "../common.h"

namespace fs = std::filesystem;

struct DirectoryNode
{
    fs::path FullPath;
    fs::path FileName;
    std::string FileSize;
    std::string LastModified;
    std::vector<DirectoryNode*> Children;
    bool IsDirectory;
};

inline DirectoryNode *rootNode;
inline DirectoryNode *selectedItem;

DirectoryNode *CreateDirectoryNodeTreeFromPath(const fs::path& rootPath);
DirectoryNode *ChangeDirectory(DirectoryNode *node);

void ReloadRootNode(DirectoryNode *node);
void HandleFileClick(DirectoryNode *node);
void DisplayDirectoryNodeRecursive(DirectoryNode *node);
void DisplayDirectoryNode(DirectoryNode *node);
void UnloadSelectedFile();
void DeleteDirectoryNodeTree(DirectoryNode* node);