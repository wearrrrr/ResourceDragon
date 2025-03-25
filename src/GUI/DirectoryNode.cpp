#include "DirectoryNode.h"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <algorithm>

inline std::string ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, const fs::path& parentPath)
{
    try 
    {
        fs::directory_iterator directoryIterator(parentPath);

        fs::path grandParentPath = parentPath.parent_path();
        if (!grandParentPath.empty() && grandParentPath != parentPath)
        {
            DirectoryNode upNode = {
                .FullPath = grandParentPath.string(),
                .FileName = "..",
                .FileSize = Utils::GetFileSize(grandParentPath),
                .LastModified = Utils::GetLastModifiedTime(grandParentPath.string()),
                .IsDirectory = true
            };
            parentNode.Children.emplace_back(upNode);
        }

        for (const auto& entry : directoryIterator)
        {

            DirectoryNode childNode = {
                .FullPath = entry.path().string(),
                .FileName = entry.path().filename().string(),
                .FileSize = Utils::GetFileSize(entry.path()),
                .LastModified = Utils::GetLastModifiedTime(entry.path().string()),
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
    catch (const fs::filesystem_error& e)
    {
        printf("Error accessing directory: %s\n", e.what());
    }
}


DirectoryNode CreateDirectoryNodeTreeFromPath(const fs::path& rootPath)
{

    DirectoryNode rootNode = {
        .FullPath = rootPath.string(),
        .FileName = rootPath.string(),
        .FileSize = Utils::GetFileSize(rootPath),
        .LastModified = Utils::GetLastModifiedTime(rootPath.string()),
        .IsDirectory = fs::is_directory(rootPath)
    };

    if (rootNode.IsDirectory)
    {
        RecursivelyAddDirectoryNodes(rootNode, rootPath);
    }

    return rootNode;
}

void ReloadRootNode(DirectoryNode& node)
{
    rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(node.FullPath));
}

void ChangeDirectory(DirectoryNode& node, DirectoryNode& rootNode)
{
    fs::path newRootPath(node.FullPath);

    if (node.FileName == "..") {
        fs::path parentPath = fs::path(rootNode.FullPath).parent_path();
        if (parentPath != fs::path(rootNode.FullPath)) {
            newRootPath = parentPath;
        } else {
            return;
        }
    }
    rootNode = CreateDirectoryNodeTreeFromPath(newRootPath);
}

void HandleFileClick(DirectoryNode& node)
{
    std::string filename = node.FileName;
    std::string ext = filename.substr(filename.find_last_of(".") + 1);

    auto [buffer, size] = read_file_to_buffer<unsigned char>(node.FullPath.c_str());

    ArchiveFormat *format = extractor_manager.getExtractorFor(buffer, size, ext);

    if (format != nullptr) {
        printf("Format: %s\n", format->getTag().c_str());
        ArchiveBase *arc = (ArchiveBase*)format->TryOpen(buffer, size);
        fs::remove_all("decrypt/");
        fs::create_directory("decrypt");

        for (int i = 0; i < arc->entries.size(); i++) {
            Entry entry = arc->entries.at(i);
            const char *data = arc->OpenStream(entry, buffer);
            std::ofstream outFile("decrypt/" + entry.name, std::ios::binary);
            outFile.write((const char*)data, entry.size);
            outFile.close();
        }
        printf("Decrypted successfully!\n");

        ReloadRootNode(rootNode);

        delete arc;

        free(buffer);
    } else {
        if (preview_state.rawContents) {
            free(preview_state.rawContents);
            preview_state.rawContents = nullptr;
        }
        Image::UnloadTexture(preview_state.texture.id);

        Image::UnloadAnimation(&preview_state.texture.anim);
        preview_state.texture.size.x = 0;
        preview_state.texture.size.y = 0;

        preview_state.rawContents = (char*)buffer;
        preview_state.rawContentsSize = size;
        preview_state.rawContentsExt = ext;

        if (Image::IsImageExtension(ext)) {
            Image::LoadTextureFromMemory(preview_state.rawContents, preview_state.rawContentsSize, &preview_state.texture.id, &preview_state.texture.size.x, &preview_state.texture.size.y);
        }
        if (Image::IsGif(ext)) {
            Image::LoadGifAnimation(preview_state.rawContents, preview_state.rawContentsSize, &preview_state.texture.anim);
            preview_state.texture.last_frame_time = SDL_GetTicks();
        }
    }
}


void DisplayDirectoryNodeRecursive(DirectoryNode& node, DirectoryNode& rootNode)
{
    ImGui::TableNextRow();
    ImGui::PushID(&node);

    bool directoryClicked = false;
    bool fileClicked = false;

    ImGui::TableNextColumn();

    bool isRoot = (&node == &rootNode);
    
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAllColumns;
    if (node.IsDirectory) {
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;
    } else {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (isRoot) { 
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing()); 
    };

    bool isOpen = ImGui::TreeNodeEx(node.FileName.c_str(), nodeFlags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        selectedItem = node;
        ImGui::OpenPopup("ContextMenu");
    }
    
    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node.IsDirectory) directoryClicked = true;
        else fileClicked = true;
    }

    ImGui::TableNextColumn();
    if (!node.IsDirectory) {
        ImGui::Text("%s", node.FileSize.c_str());
    } else {
        ImGui::Text("--");
    }


    ImGui::TableNextColumn();
    ImGui::Text("%s", node.LastModified.c_str());

    if (node.IsDirectory && isOpen) {
        for (auto& childNode : node.Children) {
            DisplayDirectoryNodeRecursive(childNode, rootNode);
        }
        ImGui::TreePop();
    }

    if (fileClicked) {
        HandleFileClick(node);
    } else if (directoryClicked) {
        ChangeDirectory(node, rootNode);
    }

    ImGui::PopID();
}


void DisplayDirectoryNode(DirectoryNode& node, DirectoryNode& rootNode, bool isRoot = false)
{
    ImGui::PushID(&node);

    ImGui::BeginTable("DirectoryTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentDisable);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 215.0f);
    ImGui::TableHeadersRow();

    DisplayDirectoryNodeRecursive(node, rootNode);

    ImGui::EndTable();

    ImGui::PopID();
}