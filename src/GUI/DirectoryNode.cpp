#include "DirectoryNode.h"

void UnloadSelectedFile() {
    if (preview_state.contents.data) {
        free(preview_state.contents.data);
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
    preview_state.audio.volume = 0;
    preview_state.audio.time = {
        .total_time_min = 0,
        .total_time_sec = 0,
        .current_time_min = 0,
        .current_time_sec = 0
    };
    if (preview_state.audio.update_timer) {
        SDL_RemoveTimer(preview_state.audio.update_timer);
        preview_state.audio.update_timer = 0;
    }
    current_sound = nullptr;
    curr_sound_is_midi = false;
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
                return Utils::ToLower(a.FileName) < Utils::ToLower(b.FileName);
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

Uint32 TimerUpdateCB(void* userdata, Uint32 interval, Uint32 param) {
    if (current_sound) {
        double current_time = Mix_GetMusicPosition(current_sound);
        preview_state.audio.time.current_time_min = (int)current_time / 60;
        preview_state.audio.time.current_time_sec = (int)current_time % 60;
    }
    return interval; // continue timer
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
        Logger::log("Decrypted successfully!");

        ReloadRootNode(rootNode);

        delete arc;

        free(buffer);
    } else {
        UnloadSelectedFile();
        preview_state.contents.data = (char*)buffer;
        preview_state.contents.size = size;
        preview_state.contents.path = node.FullPath;
        preview_state.contents.ext = ext;

        if (Image::IsImageExtension(ext)) {
            Image::LoadTextureFromMemory(preview_state.contents.data, preview_state.contents.size, &preview_state.texture.id, &preview_state.texture.size.x, &preview_state.texture.size.y);
            preview_state.content_type = "image";
        }
        if (Image::IsGif(ext)) {
            Image::LoadGifAnimation(preview_state.contents.data, preview_state.contents.size, &preview_state.texture.anim);
            preview_state.content_type = "gif";
            preview_state.texture.frame = 0;
            preview_state.texture.last_frame_time = SDL_GetTicks();
        }

        if (Audio::IsAudio(ext)) {
            // Audio::PlaySound(node.FullPath);
            // preview_state.content_type = "audio";
            Logger::log("Loading audio file: %s", node.FullPath.c_str());
            current_sound = Mix_LoadMUS(node.FullPath.c_str());
            if (current_sound) {
                Logger::log("Audio loaded successfully!");
                preview_state.content_type = "audio";
                Mix_PlayMusic(current_sound, 1);
                double duration = Mix_MusicDuration(current_sound);
                preview_state.audio.music = current_sound;
                preview_state.audio.playing = true;
                preview_state.audio.volume = Mix_VolumeMusic(-1);
                preview_state.audio.time.total_time_min = (int)duration / 60;
                preview_state.audio.time.total_time_sec = (int)duration % 60;

                preview_state.audio.update_timer = SDL_AddTimer(1000, TimerUpdateCB, nullptr);
            } else {
                Logger::error("Failed to load audio: %s", SDL_GetError());
            }
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
        ImGui::OpenPopup("FBContextMenu");
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