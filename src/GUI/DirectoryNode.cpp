#include "DirectoryNode.h"
#include "../util/Text.h"
#include "../common.h"

ArchiveBase *loaded_arc_base = nullptr;
unsigned char *current_buffer = nullptr;

void UnloadSelectedFile() {
    if (preview_state.contents.size > 0) {
        preview_state.contents.data = nullptr;
    }
    
    Image::UnloadTexture(preview_state.texture.id);
    Image::UnloadAnimation(&preview_state.texture.anim);
    
    preview_state.texture = {
        .id = 0,
        .size = {0, 0},
        .anim = {},
        .frame = 0,
        .last_frame_time = 0
    };

    preview_state.content_type = "";

    preview_state.contents = {
        .data = nullptr,
        .size = 0,
        .path = "",
        .ext = ""
    };

    if (preview_state.audio.music) {
        Mix_FreeMusic(preview_state.audio.music);
        preview_state.audio.music = nullptr;
    }
    preview_state.audio.playing = false;
    preview_state.audio.time = {
        .total_time_min = 0,
        .total_time_sec = 0,
        .current_time_min = 0,
        .current_time_sec = 0,
    };
    preview_state.audio.scrubberDragging = false;
    if (preview_state.audio.update_timer) {
        SDL_RemoveTimer(preview_state.audio.update_timer);
        preview_state.audio.update_timer = 0;
    }
    current_sound = nullptr;
    curr_sound_is_midi = false;
}

void DeleteDirectoryNodeTree(DirectoryNode* node)
{
    if (!node) return;

    for (DirectoryNode *child : node->Children) {
        DeleteDirectoryNodeTree(child);
    }

    delete node;
}

inline void CreateUpNode(DirectoryNode *node, const fs::path grandParentPath) {
    DirectoryNode *upNode = new DirectoryNode {
        .FullPath = grandParentPath.string(),
        .FileName = "..",
        .FileSize = "--",
        .LastModified = "N/A",
        .IsDirectory = true
    };
    node->Children.push_back(upNode);
}

bool AddDirectoryNodes(DirectoryNode *node, const fs::path &parentPath)
{
    try 
    {
        fs::path grandParentPath = parentPath.parent_path();
        if (!grandParentPath.empty() && grandParentPath != parentPath)
        {
            CreateUpNode(node, grandParentPath);
        }

        if (node->IsVirtualRoot) {
            std::vector<Entry *> entries = loaded_arc_base->GetEntries();
            for (const auto entry : entries) {
                std::replace(entry->name.begin(), entry->name.end(), '\\', '/');
                DirectoryNode *current = node;
                
                DirectoryNode *fileNode = new DirectoryNode {
                    .FullPath = entry->name,
                    .FileName = entry->name,
                    .FileSize = Utils::GetFileSize(entry->size),
                    .LastModified = "Unknown",
                    .IsDirectory = false,
                };
                current->Children.push_back(fileNode);
            }
        
            return true;
        }

        fs::directory_iterator directoryIterator(parentPath);
        for (const auto &entry : directoryIterator) {
            DirectoryNode *childNode = new DirectoryNode {
                .FullPath = entry.path().string(),
                .FileName = entry.path().filename().string(),
                .FileSize = Utils::GetFileSize(entry.path()),
                .LastModified = Utils::GetLastModifiedTime(entry.path().string()),
                .IsDirectory = entry.is_directory()
            };
            node->Children.push_back(childNode);
        }
        



        std::sort(node->Children.begin(), node->Children.end(), 
        [](const DirectoryNode* a, const DirectoryNode* b) 
        {
            if (a->FileName == "..") return true;
            if (b->FileName == "..") return false;
                if (a->IsDirectory != b->IsDirectory) return a->IsDirectory > b->IsDirectory;
                return Utils::ToLower(a->FileName) < Utils::ToLower(b->FileName);
            }
        );

        return true;
    } catch (const fs::filesystem_error &e) {
            const char *errorMessage = e.what();
            printf("Error accessing directory: %s\n", errorMessage);
            // Show errorMessage in the GUI
            ui_error.message = errorMessage;
            ui_error.title = "Error accessing directory!";
            ui_error.show = true;
            return false;
    }
}


DirectoryNode *CreateDirectoryNodeTreeFromPath(const std::string& rootPath)
{
    bool is_dir = fs::is_directory(rootPath);
    DirectoryNode *newRootNode = new DirectoryNode {
        .FullPath = rootPath,
        .FileName = rootPath,
        .FileSize = Utils::GetFileSize(rootPath),
        .LastModified = Utils::GetLastModifiedTime(rootPath),
        .IsDirectory = is_dir,
        .IsVirtualRoot = !is_dir,
    };

    bool add = AddDirectoryNodes(newRootNode, rootPath);
    if (newRootNode->IsDirectory)
    {
        if (!add) {
            // TODO: fs::path is bad.
            // ideally I would roll my own path parser but I don't think I'm masochistic enough to do that.
            newRootNode = CreateDirectoryNodeTreeFromPath(fs::path(newRootNode->FullPath).parent_path().string());
        }
    }

    return newRootNode;
}

void ReloadRootNode(DirectoryNode *node)
{
    rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(node->FullPath).string());
}

Uint32 TimerUpdateCB(void* userdata, Uint32 interval, Uint32 param) {
    if (current_sound) {
        int current_time = (int)Mix_GetMusicPosition(current_sound);
        if (!preview_state.audio.scrubberDragging) {
            preview_state.audio.time.current_time_min = current_time / 60;
            preview_state.audio.time.current_time_sec = current_time % 60;
        }
    }
    return interval;
}

bool CreateDirectoryRecursive(const std::string &dirName, std::error_code &err)
{
    err.clear();
    if (!std::filesystem::create_directories(dirName, err))
    {
        if (std::filesystem::exists(dirName))
        {
            err.clear();
            return true;    
        }
        return false;
    }
    return true;
}

void HandleFileClick(DirectoryNode *node)
{
    std::string filename = node->FileName;
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    unsigned char *buffer;
    size_t size = 0;

    if (rootNode->IsVirtualRoot && !node->IsDirectory) {
        auto entries = loaded_arc_base->GetEntries();

        Entry *found_entry = nullptr;

        for (const auto &entry : entries) {
            if (entry->name == node->FullPath) {
                found_entry = entry;
            }
        }

        if (found_entry) {
            if (current_buffer) {
                const char *arc_read = loaded_arc_base->OpenStream(found_entry, current_buffer);
                buffer = (unsigned char*)arc_read;
                size = found_entry->size;
            }
        }
    } else {
        auto [fs_buffer, fs_size] = read_file_to_buffer<unsigned char>(node->FullPath.c_str());
        buffer = fs_buffer;
        size = fs_size;
    }

    

    ArchiveFormat *format = extractor_manager.getExtractorFor(buffer, size, ext);

    if (format == nullptr) {
        UnloadSelectedFile();
        preview_state.contents.data = buffer;
        preview_state.contents.size = size;
        preview_state.contents.path = node->FullPath;
        preview_state.contents.ext = ext;

        if (Image::IsImageExtension(ext)) {
            Image::LoadTextureFromMemory(preview_state.contents.data, preview_state.contents.size, &preview_state.texture.id, &preview_state.texture.size.x, &preview_state.texture.size.y);
            preview_state.content_type = "image";
        } else if (Image::IsGif(ext)) {
            Image::LoadGifAnimation(preview_state.contents.data, preview_state.contents.size, &preview_state.texture.anim);
            preview_state.content_type = "gif";
            preview_state.texture.frame = 0;
            preview_state.texture.last_frame_time = SDL_GetTicks();
        } else if (Audio::IsAudio(ext)) {
            std::string path = node->FullPath.c_str();
            if (rootNode->IsVirtualRoot) {
                SDL_IOStream * snd_io = SDL_IOFromConstMem(buffer, size);
                current_sound = Mix_LoadMUS_IO(snd_io, true);
            } else {
                current_sound = Mix_LoadMUS(path.c_str());
            }
            if (current_sound) {
                preview_state.content_type = "audio";
                Mix_PlayMusic(current_sound, 1);
                int duration = (int)Mix_MusicDuration(current_sound);
                preview_state.audio.music = current_sound;
                preview_state.audio.playing = true;
                preview_state.audio.time.total_time_min = duration / 60;
                preview_state.audio.time.total_time_sec = duration % 60;
                preview_state.audio.update_timer = SDL_AddTimer(1000, TimerUpdateCB, nullptr);
            } else {
                Logger::error("Failed to load audio: %s", SDL_GetError());
            }
        } else if (ElfFile::IsValid(buffer)) {
            ElfFile *elfFile = new ElfFile(node->FullPath);
            preview_state.contents.elfFile = elfFile;
            preview_state.content_type = "elf";
        } else {
            auto text = std::string((char*)buffer, size);
            // Check start of file for UTF16LE BOM
            if (text.size() >= 2 && text[0] == '\xFF' && text[1] == '\xFE') {
                preview_state.contents.conv_data = TextConverter::UTF16LEToUTF8(text);
                editor.SetText(preview_state.contents.conv_data);
            } else {
                editor.SetText(text);
            }
            editor.SetTextChanged(false);
            editor.SetColorizerEnable(true);
        }
        return;
    }

    ArchiveBase *arc = format->TryOpen(buffer, size, node->FileName);
    if (arc == nullptr) {
        Logger::error("Failed to open archive: %s! Attempted to open as: %s", node->FileName.c_str(), format->getTag().c_str());
        goto hfc_end;
    }

    loaded_arc_base = arc;
    current_buffer = (unsigned char*)malloc(size);
    memcpy(current_buffer, buffer, size);

    rootNode = CreateDirectoryNodeTreeFromPath(node->FullPath);
    rootNode->IsVirtualRoot = true;

hfc_end:
    free(buffer);
    return;
}

std::optional<std::string> pathToReopen;

void DisplayDirectoryNode(DirectoryNode *node)
{
    ImGui::TableNextRow();
    ImGui::PushID(node);

    bool directoryClicked = false;
    bool fileClicked = false;

    ImGui::TableNextColumn();

    bool isRoot = (node == rootNode);
    
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAllColumns;
    node->IsDirectory 
        ? nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow 
        : nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (isRoot) { 
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;
        ImGui::Unindent(30.0f); 
    };

    bool isOpen = ImGui::TreeNodeEx(node->FileName.c_str(), nodeFlags);

    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node->IsDirectory) { 
            pathToReopen = node->FullPath;
            if (node->IsVirtualRoot) {
                rootNode = node;
            }
        } else HandleFileClick(node);
    }
    
    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node->IsDirectory) directoryClicked = true;
        else fileClicked = true;
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        selectedItem = node;
        ImGui::OpenPopup("FBContextMenu");
    }

    ImGui::TableNextColumn();
    ImGui::Text("%s", node->FileSize.c_str());

    ImGui::TableNextColumn();
    ImGui::Text("%s", node->LastModified.c_str());

    if (node->IsVirtualRoot) {
        for (auto childNode : node->Children) {
            DisplayDirectoryNode(childNode);
        }
    }

    if (node->IsDirectory && isOpen) {
        for (auto childNode : node->Children) {
            DisplayDirectoryNode(childNode);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}


void SetupDisplayDirectoryNode(DirectoryNode *node)
{
    ImGui::PushID(node);

    ImGui::BeginTable("DirectoryTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentDisable);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 170.0f);
    ImGui::TableHeadersRow();

    DisplayDirectoryNode(node);

    if (pathToReopen.has_value()) {
        DeleteDirectoryNodeTree(rootNode);
        rootNode = CreateDirectoryNodeTreeFromPath(*pathToReopen);
        pathToReopen.reset();
    }

    ImGui::EndTable();

    ImGui::PopID();
}
