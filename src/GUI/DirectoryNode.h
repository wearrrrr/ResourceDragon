#pragma once

#include <filesystem>
#include <ArchiveFormats/Entry.h>

namespace fs = std::filesystem;

#include <string>
#include <vector>
#include <util/int.h>

namespace DirectoryNode {
    struct Node {
        std::string FullPath = "";
        std::string FileName = "";
        std::string FileSize = "";
        u64 FileSizeBytes = 0;
        std::string LastModified = "";
        u64 LastModifiedUnix = 0;
        DirectoryNode::Node *Parent = nullptr;
        std::vector<DirectoryNode::Node*> Children = {};
        bool IsDirectory = false;
        bool IsVirtualRoot = false;
    };

    Node *CreateTreeFromPath(const std::string& rootPath, DirectoryNode::Node *parent = nullptr);

    bool AddNodes(Node *node, const fs::path &parentPath);
    void ReloadRootNode(Node *node);
    void HandleFileClick(Node *node);
    void Display(Node *node);
    void Setup(Node *node);
    void UnloadSelectedFile();
    void Unload(Node* node);
}

void SetFilePath(const std::string& file_path);

inline DirectoryNode::Node *rootNode;
inline DirectoryNode::Node *selectedItem;



namespace VirtualArc {
    bool ExtractEntry(const fs::path &path, Entry *entry, fs::path outputPath = {});
    void ExtractEntry(std::string path = "extracted/");
    void ExtractAll();
}
