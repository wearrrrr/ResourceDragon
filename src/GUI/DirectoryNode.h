#pragma once

#include <string>
#include <vector>

struct DirectoryNode
{
    std::string FullPath;
    std::string FileName;
    std::string FileSize;
    std::string LastModified;
    std::vector<DirectoryNode*> Children;
    bool IsDirectory;
};

inline DirectoryNode *rootNode;
inline DirectoryNode *selectedItem;

DirectoryNode *CreateDirectoryNodeTreeFromPath(const std::string& rootPath);

void ReloadRootNode(DirectoryNode *node);
void HandleFileClick(DirectoryNode *node);
void DisplayDirectoryNodeRecursive(DirectoryNode *node);
void DisplayDirectoryNode(DirectoryNode *node);
void UnloadSelectedFile();
void DeleteDirectoryNodeTree(DirectoryNode* node);
