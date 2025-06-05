#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct DirectoryNode
{
    std::string FullPath = "";
    std::string FileName = "";
    std::string FileSize = "";
    uint64_t FileSizeBytes = 0;
    std::string LastModified = "";
    uint64_t LastModifiedUnix = 0;
    DirectoryNode *Parent = nullptr;
    std::vector<DirectoryNode*> Children = {};
    bool IsDirectory = false;
    bool IsVirtualRoot = false;
};

inline DirectoryNode *rootNode;
inline DirectoryNode *selectedItem;

DirectoryNode *CreateDirectoryNodeTreeFromPath(const std::string& rootPath, DirectoryNode *parent = nullptr);

void ReloadRootNode(DirectoryNode *node);
void HandleFileClick(DirectoryNode *node);
void DisplayDirectoryNode(DirectoryNode *node);
void SetupDisplayDirectoryNode(DirectoryNode *node);
void UnloadSelectedFile();
void FreeDirectoryTree(DirectoryNode* node);

void VirtualArc_ExtractEntry();
