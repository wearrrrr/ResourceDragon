#include <cstring>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <functional>

#include <Audio.h>
#include <DirectoryNode.h>
#include <Utils.h>
#include "../state.h"
#include "../util/Text.h"
#include "gl3.h"

Entry* FindEntryByNode(const std::unordered_map<std::string, Entry*> &entries, const DirectoryNode *node) {
    for (const auto &entry : entries) {
        if (node->FullPath.contains(entry.second->name)) {
            return entry.second;
        }
    }
    return nullptr;
}


bool CreateDirectoryRecursive(const std::string &dirName, std::error_code &err) {
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

void VirtualArc_ExtractEntry() {
    if (!loaded_arc_base) {
        Logger::error("No loaded archive! This is a bug!");
    }
    if (!selected_entry) {
        Logger::error("No selected entry found! This is a bug!");
        return;
    }
    if (!current_buffer) {
        Logger::error("current_buffer is not initialized! This is a bug!");
        return;
    }

    const char *extracted = loaded_arc_base->OpenStream(selected_entry, current_buffer);

    Logger::log("%s", selected_entry->name.c_str());

    #ifdef linux
    std::replace(selected_entry->name.begin(), selected_entry->name.end(), '\\', '/');
    #endif


    fs::path entryPath(selected_entry->name);
    std::error_code err;
    std::string extraPath = "";
    if (entryPath.has_parent_path()) {
        auto parentDir = entryPath.parent_path().string();
        extraPath = parentDir;
    }
    if (!CreateDirectoryRecursive("extracted/" + extraPath, err)) {
        Logger::error("Failed to create directory: %s", err.message().c_str());
    }

    std::ofstream outFile("extracted/" + selected_entry->name, std::ios::binary);
    outFile.write(extracted, selected_entry->size);
    outFile.close();
}

void UnloadSelectedFile() {
    if (preview_state.contents.size > 0) {
        preview_state.contents.data = nullptr;
    }

    Image::UnloadTexture(preview_state.texture.id);
    Image::UnloadAnimation(&preview_state.texture.anim);

    preview_state.texture.id = 0;
    preview_state.texture.size = {0, 0};
    preview_state.texture.anim = {};
    preview_state.texture.frame = 0;
    preview_state.texture.last_frame_time = 0;
    preview_state.texture.firstFrame = true;

    preview_state.content_type = PContentType::UNKNOWN;

    preview_state.contents.data = nullptr;
    preview_state.contents.size = 0;
    preview_state.contents.path = "";
    preview_state.contents.ext = "";
    preview_state.contents.elf_header = {};
    preview_state.contents.elfFile = nullptr;

    if (preview_state.audio.music) {
        Mix_FreeMusic(preview_state.audio.music);
        preview_state.audio.music = nullptr;
    }
    preview_state.audio.buffer = nullptr;
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

void DeleteDirectoryNodeTree(DirectoryNode* node) {
    if (!node) return;

    for (DirectoryNode *child : node->Children) {
        DeleteDirectoryNodeTree(child);
    }

    delete node;
}

inline void SortChildren(DirectoryNode *node) {
    std::sort(node->Children.begin(), node->Children.end(),
    [](const DirectoryNode* a, const DirectoryNode* b) {
        if (a->IsDirectory != b->IsDirectory) return a->IsDirectory > b->IsDirectory;

        return Utils::ToLower(a->FileName) < Utils::ToLower(b->FileName);
    });
}

bool AddDirectoryNodes(DirectoryNode *node, const fs::path &parentPath) {
    try {
        if (node->IsVirtualRoot) {
            auto entries = loaded_arc_base->GetEntries();

            for (const auto &entry : entries) {
                #ifdef linux
                std::replace(entry.second->name.begin(), entry.second->name.end(), '\\', '/');
                #endif

                fs::path entryPath(entry.second->name);
                DirectoryNode *current = node;

                for (auto it = entryPath.begin(); it != entryPath.end(); ++it) {
                    std::string part = it->string();
                    bool isLast = (std::next(it) == entryPath.end());

                    auto found = std::find_if(current->Children.begin(), current->Children.end(),
                        [&part](const DirectoryNode* child) {
                            return child->FileName == part;
                        }
                    );

                    if (found == current->Children.end()) {
                        DirectoryNode *newNode = new DirectoryNode{
                            .FullPath = (current->FullPath.empty() ? part : current->FullPath + "/" + part),
                            .FileName = part,
                            .FileSize = isLast ? Utils::GetFileSize(entry.second->size) : "--",
                            .LastModified = isLast ? "Unknown" : "N/A",
                            .Children = {},
                            .IsDirectory = !isLast
                        };
                        current->Children.push_back(newNode);
                        current = newNode;
                    } else {
                        current = *found;
                    }
                }
            }
        } else {
            fs::directory_iterator directoryIterator(parentPath);
            for (const auto &entry : directoryIterator) {
                std::string path = entry.path().string();
                DirectoryNode *childNode = new DirectoryNode {
                    .FullPath = path,
                    .FileName = entry.path().filename().string(),
                    .FileSize = Utils::GetFileSize(path),
                    .FileSizeBytes = entry.is_directory() ? 0 : fs::file_size(entry),
                    .LastModified = Utils::GetLastModifiedTime(path),
                    .LastModifiedUnix = (uint64_t)(fs::last_write_time(entry).time_since_epoch().count()),
                    .Children = {},
                    .IsDirectory = entry.is_directory()
                };
                node->Children.push_back(childNode);
            }
        }
        SortChildren(node);
        return true;
    } catch (const fs::filesystem_error &e) {
            const char *errorMessage = e.what();
            printf("Error accessing directory: %s\n", errorMessage);
            ui_error.message = errorMessage;
            ui_error.title = "Error accessing directory!";
            ui_error.show = true;
            return false;
    }
}


DirectoryNode *CreateDirectoryNodeTreeFromPath(const std::string& rootPath, DirectoryNode *parent) {
    bool is_dir = fs::is_directory(rootPath);
    DirectoryNode *newRootNode = new DirectoryNode {
        .FullPath = rootPath,
        .FileName = rootPath,
        .FileSize = Utils::GetFileSize(rootPath),
        .FileSizeBytes = is_dir ? 0 : fs::file_size(rootPath),
        .LastModified = Utils::GetLastModifiedTime(rootPath),
        .LastModifiedUnix = (uint64_t)fs::last_write_time(rootPath).time_since_epoch().count(),
        .Parent = parent,
        .Children = {},
        .IsDirectory = is_dir,
        .IsVirtualRoot = !is_dir,
    };

    if (is_dir || newRootNode->IsVirtualRoot) {
        AddDirectoryNodes(newRootNode, rootPath);
    }

    return newRootNode;
}

void ReloadRootNode(DirectoryNode *node) {
    if (fs::is_directory(node->FullPath)) {
        rootNode = CreateDirectoryNodeTreeFromPath(fs::canonical(node->FullPath).string());
    }
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

void HandleFileClick(DirectoryNode *node) {
    std::string filename = node->FileName;
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    unsigned char* buffer = nullptr;
    size_t size = 0;

    if (rootNode->IsVirtualRoot && !node->IsDirectory) {
        selected_entry = FindEntryByNode(loaded_arc_base->GetEntries(), node);

        if (selected_entry) {
            if (current_buffer) {
                const char *arc_read = loaded_arc_base->OpenStream(selected_entry, current_buffer);
                size = selected_entry->size;
                buffer = (unsigned char *)malloc(size);
                memcpy(buffer, arc_read, size);
            } else {
                Logger::error("current_buffer is not initialized, aborting.");
                return;
            }
        }
    } else {
        auto [fs_buffer, fs_size] = read_file_to_buffer<unsigned char>(node->FullPath.c_str());
        size = fs_size;
        buffer = fs_buffer;
    }

    if (buffer == nullptr) {
        Logger::error("Unable to resolve what buffer should be! Aborting.");
        return;
    }

    auto format = extractor_manager.getExtractorFor(buffer, size, ext);

    if (format == nullptr) {
        UnloadSelectedFile();
        preview_state.contents.data = buffer;
        preview_state.contents.size = size;
        preview_state.contents.path = node->FullPath;
        preview_state.contents.ext = ext;
        preview_state.contents.fileName = node->FileName;

        if (Image::IsImageExtension(ext)) {
            Image::LoadImage(buffer, size, &preview_state.texture.id, {&preview_state.texture.size.x, &preview_state.texture.size.y});
            if (preview_state.texture.size.x < 256) {
                // reload img with nearest neighbor filtering
                Image::LoadImage(buffer, size, &preview_state.texture.id, {&preview_state.texture.size.x, &preview_state.texture.size.y}, GL_NEAREST);
            }
            preview_state.content_type = IMAGE;
        } else if (Image::IsGif(ext)) {
            Image::LoadGifAnimation(preview_state.contents.data, preview_state.contents.size, &preview_state.texture.anim);
            preview_state.content_type = GIF;
            preview_state.texture.frame = 0;
            preview_state.texture.last_frame_time = SDL_GetTicks();
        } else if (Audio::IsAudio(ext)) {
            if (rootNode->IsVirtualRoot) {
                preview_state.audio.buffer = buffer;
                SDL_IOStream *snd_io = SDL_IOFromConstMem(preview_state.audio.buffer, size);
                current_sound = Mix_LoadMUS_IO(snd_io, true);
            } else {
                current_sound = Mix_LoadMUS(node->FullPath.c_str());
            }
            if (current_sound) {
                preview_state.content_type = AUDIO;
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
            preview_state.content_type = ELF;
        } else {
            auto text = std::string((char*)buffer, size);
            // Check start of file for UTF16LE BOM
            if (text.size() >= 2 && text[0] == '\xFF' && text[1] == '\xFE') {
                std::u16string utf16((char16_t*)text.data() + 1, (text.size() - 1) / 2);
                editor.SetText(TextConverter::UTF16ToUTF8(utf16));
            } else {
                editor.SetText(text);
            }
            editor.SetTextChanged(false);
            if (size <= 200000) {
                editor.SetColorizerEnable(true);
            } else {
                editor.SetColorizerEnable(false);
            }
            editor.SetShowWhitespaces(false);
        }
        return;
    }

    auto arc = format->TryOpen(buffer, size, node->FileName);
    if (arc == nullptr) {
        Logger::error("Failed to open archive: %s! Attempted to open as: %s", node->FileName.c_str(), format->GetTag().c_str());
        goto hfc_end;
    }
    if (loaded_arc_base) {
        delete loaded_arc_base;
    }
    loaded_arc_base = arc;

    if (current_buffer) {
        free(current_buffer);
    }
    current_buffer = (unsigned char *)malloc(size);
    memcpy(current_buffer, buffer, size);

    rootNode = CreateDirectoryNodeTreeFromPath(node->FullPath);

hfc_end:
    if (buffer) {
        free(buffer);
    }
    return;
}

void DisplayDirectoryNode(DirectoryNode *node) {
    ImGui::TableNextRow();
    ImGui::PushID(node);

    ImGui::TableNextColumn();
    ImGui::Selectable(node->FileName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);

    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node->IsDirectory) {
            if (rootNode->IsVirtualRoot) {
                node->IsVirtualRoot = true;
                node->Parent = rootNode;
                rootNode = node;
            } else {
                rootNode = CreateDirectoryNodeTreeFromPath(node->FullPath, rootNode);
            }
        } else {
            HandleFileClick(node);
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        selectedItem = node;
        if (loaded_arc_base) {
            selected_entry = FindEntryByNode(loaded_arc_base->GetEntries(), node);
        }
        ImGui::OpenPopup("FBContextMenu");
    }

    ImGui::TableNextColumn();
    ImGui::Text("%s", node->FileSize.c_str());

    ImGui::TableNextColumn();
    ImGui::Text("%s", node->LastModified.c_str());

    ImGui::PopID();
}

void AddDirectoryNodeChild(std::string name, std::function<void()> callback = nullptr) {
    if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (callback) callback();
    };
}

#define FB_COLUMNS 3
void SetupDisplayDirectoryNode(DirectoryNode *node) {
    ImGui::PushID(node);

    ImGui::BeginTable("DirectoryTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentDisable);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 170.0f);
    ImGui::TableNextRow();

    for (int col = 0; col < FB_COLUMNS; col++) {
        ImGui::TableSetColumnIndex(col);
        const char* name = ImGui::TableGetColumnName(col);
        ImGui::PushID(col);
        if (ImGui::Selectable(name, false)) {
            if (strcmp(name, "Size") == 0) {
                std::sort(node->Children.begin(), node->Children.end(), [](const DirectoryNode* a, const DirectoryNode* b) {
                    return a->FileSizeBytes > b->FileSizeBytes;
                });
            } else if (strcmp(name, "Last Modified") == 0) {
                std::sort(node->Children.begin(), node->Children.end(), [](const DirectoryNode* a, const DirectoryNode* b) {
                    return a->LastModifiedUnix < b->LastModifiedUnix;
                });
            } else {
                SortChildren(node);
            }
        }
        ImGui::PopID();
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    AddDirectoryNodeChild(node->FullPath);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    AddDirectoryNodeChild("..", [&node](){
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (node->Parent) {
                rootNode = node->Parent;
            } else {
                rootNode = CreateDirectoryNodeTreeFromPath(fs::path(node->FullPath).parent_path().string());
                if (current_buffer) {
                    free(current_buffer);
                    current_buffer = nullptr;
                }
            }
        }
    });
    ImGui::TableNextColumn();

    for (auto childNode : node->Children) {
        DisplayDirectoryNode(childNode);
    }

    ImGui::EndTable();
    ImGui::PopID();
}
