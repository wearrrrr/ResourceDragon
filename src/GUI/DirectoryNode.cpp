#include "DirectoryNode.h"
#include "../common.h"

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

    for (DirectoryNode* child : node->Children) {
        DeleteDirectoryNodeTree(child);
    }

    delete node;
}

bool AddDirectoryNodes(DirectoryNode *parentNode, const fs::path& parentPath)
{
    try 
    {
        fs::directory_iterator directoryIterator(parentPath);
        fs::path grandParentPath = parentPath.parent_path();
        if (!grandParentPath.empty() && grandParentPath != parentPath)
        {
            DirectoryNode *upNode = new DirectoryNode {
                .FullPath = grandParentPath.string(),
                .FileName = "..",
                .FileSize = Utils::GetFileSize(grandParentPath),
                .LastModified = Utils::GetLastModifiedTime(grandParentPath.string()),
                .IsDirectory = true
            };
            parentNode->Children.push_back(upNode);
        }

        for (const auto& entry : directoryIterator) {

            DirectoryNode *childNode = new DirectoryNode {
                .FullPath = entry.path().string(),
                .FileName = entry.path().filename().string(),
                .FileSize = Utils::GetFileSize(entry.path()),
                .LastModified = Utils::GetLastModifiedTime(entry.path().string()),
                .IsDirectory = entry.is_directory()
            };

            parentNode->Children.push_back(childNode);
        }

        std::sort(parentNode->Children.begin(), parentNode->Children.end(), 
        [](const DirectoryNode* a, const DirectoryNode* b) 
        {
            if (a->FileName == "..") return true;
            if (b->FileName == "..") return false;
                if (a->IsDirectory != b->IsDirectory) return a->IsDirectory > b->IsDirectory;
                return Utils::ToLower(a->FileName) < Utils::ToLower(b->FileName);
            }
        );

        return true;
    } catch (const fs::filesystem_error& e) {
            const char* errorMessage = e.what();
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

    DirectoryNode *rootNode = new DirectoryNode {
        .FullPath = rootPath,
        .FileName = rootPath,
        .FileSize = Utils::GetFileSize(rootPath),
        .LastModified = Utils::GetLastModifiedTime(rootPath),
        .IsDirectory = fs::is_directory(rootPath)
    };

    if (rootNode->IsDirectory)
    {
        bool add = AddDirectoryNodes(rootNode, rootPath);
        if (!add) {
            rootNode = CreateDirectoryNodeTreeFromPath(fs::path(rootNode->FullPath).parent_path().string());
        }
    }

    return rootNode;
}

void ReloadRootNode(DirectoryNode *node)
{
    rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(node->FullPath).string());
}

DirectoryNode *ChangeDirectory(DirectoryNode *node)
{
    std::string newRootPath(node->FullPath);

    Logger::log("%s", newRootPath.c_str());
    Logger::log("%s", rootNode->FullPath.c_str());

    #ifdef linux
    if (inotify_fd >= 0) {
        inotify_rm_watch(inotify_fd, inotify_wd);
        inotify_wd = inotify_add_watch(inotify_fd, newRootPath.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    }
    #endif

    if (node->FileName == "..") {
        fs::path parentPath = fs::path(rootNode->FullPath).parent_path();
        if (parentPath != fs::path(rootNode->FullPath)) {
            newRootPath = parentPath.string();
        } else {
            return rootNode;
        }
    }
    
    return CreateDirectoryNodeTreeFromPath(newRootPath);
}

Uint32 TimerUpdateCB(void* userdata, Uint32 interval, Uint32 param) {
    if (current_sound) {
        double current_time = Mix_GetMusicPosition(current_sound);
        if (!preview_state.audio.scrubberDragging) {
            preview_state.audio.time.current_time_min = (int)current_time / 60;
            preview_state.audio.time.current_time_sec = (int)current_time % 60;
        }
    }
    return interval; // continue timer
}

bool CreateDirectoryRecursive(std::string const &dirName, std::error_code &err)
{
    err.clear();
    if (!std::filesystem::create_directories(dirName, err))
    {
        if (std::filesystem::exists(dirName))
        {
            // The folder already exists:
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

    auto [buffer, size] = read_file_to_buffer<unsigned char>(node->FullPath.c_str());

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
            Logger::log("Loading audio file: %s", node->FullPath.c_str());
            current_sound = Mix_LoadMUS(node->FullPath.c_str());
            if (current_sound) {
                Logger::log("Audio loaded successfully!");
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
            editor.SetColorizerEnable(false);
        }
        return;
    }

    ArchiveBase *arc = format->TryOpen(buffer, size, node->FileName);
    if (arc == nullptr) {
        Logger::error("Failed to open archive: %s! Attempted to open as: %s", node->FileName.c_str(), format->getTag().c_str());
        free(buffer);
        return;
    }

    Logger::log("Handling archive as %s", format->getTag().c_str());

    fs::remove_all("decrypt/");
    fs::create_directory("decrypt");

    for (int i = 0; i < arc->GetEntries().size(); i++) {

        Entry *entry = arc->GetEntries().at(i);
        const char *data = arc->OpenStream(entry, buffer);

        // Create directory from entry.name if it doesn't exist
        fs::path entryPath(entry->name);
        if (entryPath.has_parent_path()) {
            std::string parentDir = entryPath.parent_path().string();
            std::error_code err;
            if (!CreateDirectoryRecursive("decrypt/" + parentDir, err)) {
                Logger::error("Failed to create directory: %s", err.message().c_str());
                continue;
            }
        }
        
        std::ofstream outFile("decrypt/" + entry->name, std::ios::binary);
        outFile.write((const char*)data, entry->size);
        outFile.close();
    }
    Logger::log("Decrypted successfully!");

    ReloadRootNode(rootNode);

    delete arc;

    free(buffer);
    return;
}

std::optional<std::string> pathToReopen;

void DisplayDirectoryNodeRecursive(DirectoryNode *node)
{
    ImGui::TableNextRow();
    ImGui::PushID(node);

    bool directoryClicked = false;
    bool fileClicked = false;

    ImGui::TableNextColumn();

    bool isRoot = (node == rootNode);
    
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAllColumns;
    if (node->IsDirectory) {
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;
    } else {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (isRoot) { 
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;
        ImGui::Unindent(30.0f); 
    };

    bool isOpen = ImGui::TreeNodeEx(node->FileName.c_str(), nodeFlags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        selectedItem = node;
        ImGui::OpenPopup("FBContextMenu");
    }

    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node->IsDirectory) {
            pathToReopen = node->FullPath;
        } else {
            HandleFileClick(node);
        }
    }
    
    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node->IsDirectory) directoryClicked = true;
        else fileClicked = true;
    }

    ImGui::TableNextColumn();
    if (!node->IsDirectory) {
        ImGui::Text("%s", node->FileSize.c_str());
    } else {
        ImGui::Text("--");
    }


    ImGui::TableNextColumn();
    ImGui::Text("%s", node->LastModified.c_str());

    if (node->IsDirectory && isOpen) {
        for (auto childNode : node->Children) {
            DisplayDirectoryNodeRecursive(childNode);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}


void DisplayDirectoryNode(DirectoryNode *node)
{
    ImGui::PushID(node);

    ImGui::BeginTable("DirectoryTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentDisable);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 170.0f);
    ImGui::TableHeadersRow();

    DisplayDirectoryNodeRecursive(node);

    if (pathToReopen.has_value()) {
        DeleteDirectoryNodeTree(rootNode);
        rootNode = CreateDirectoryNodeTreeFromPath(*pathToReopen);
        pathToReopen.reset();
    }

    ImGui::EndTable();

    ImGui::PopID();
}
