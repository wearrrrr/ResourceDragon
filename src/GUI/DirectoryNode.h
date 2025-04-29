#pragma once

#include <string>
#include <vector>
#include <sstream>

struct DirectoryNode
{
    std::string FullPath;
    std::string FileName;
    std::string FileSize;
    std::string LastModified;
    std::vector<DirectoryNode*> Children;
    bool IsDirectory;
    bool IsVirtualRoot;
};

inline DirectoryNode *rootNode;
inline DirectoryNode *selectedItem;

DirectoryNode *CreateDirectoryNodeTreeFromPath(const std::string& rootPath);

void ReloadRootNode(DirectoryNode *node);
void HandleFileClick(DirectoryNode *node);
void DisplayDirectoryNode(DirectoryNode *node);
void SetupDisplayDirectoryNode(DirectoryNode *node);
void UnloadSelectedFile();
void DeleteDirectoryNodeTree(DirectoryNode* node);

void VirtualArc_ExtractEntry();
