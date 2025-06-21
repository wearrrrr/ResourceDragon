#pragma once

#include <filesystem>
#include <ArchiveFormats/Entry.h>

namespace fs = std::filesystem;

#include <string>
#include <vector>
#include <util/int.h>

struct DirectoryNode
{
    std::string FullPath = "";
    std::string FileName = "";
    std::string FileSize = "";
    u64 FileSizeBytes = 0;
    std::string LastModified = "";
    u64 LastModifiedUnix = 0;
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
void SetFilePath(const std::string& file_path);
void SetupDisplayDirectoryNode(DirectoryNode *node);
void UnloadSelectedFile();
void FreeDirectoryTree(DirectoryNode* node);

void VirtualArc_ExtractEntry(fs::path path, Entry *entry, fs::path outputPath = {});
void VirtualArc_ExtractEntry(std::string path = "extracted/");
void VirtualArc_ExtractAll();
