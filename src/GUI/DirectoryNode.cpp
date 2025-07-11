#include <Audio.h>
#include <DirectoryNode.h>
#include <Image.h>
#include <cmath>
#include <filesystem>
#include <string>
#include <util/Text.h>
#include <util/int.h>
#include <Utils.h>

#include "UIError.h"
#include "imgui.h"
#include "state.h"
#include "gl3.h"

#include "icons.h"

void InfoDialog() {
    if (ImGui::BeginPopupModal("Format Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {

        if (ImGui::BeginTable("FormatsTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 250.0f);
            ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthStretch, 250.0f);
            ImGui::TableHeadersRow();

            for (const auto& pair : extractor_manager.GetFormats()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Selectable(pair.first.c_str(), false);
                ImGui::TableSetColumnIndex(1);
                ImGui::Selectable(pair.second->GetDescription().c_str(), false);
            }

            ImGui::EndTable();
        }


        if (ImGui::Button("Close", {100, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

Entry* FindEntryByNode(const EntryMapPtr &entries, const DirectoryNode::Node *node) {
    const std::string& fullPath = node->FullPath;
    for (const auto &entry : entries) {
        const std::string& name = entry.second->name;

        if (fullPath.size() >= name.size() && fullPath.compare(fullPath.size() - name.size(), name.size(), name) == 0) {
            char lastChar = fullPath[fullPath.size() - name.size() - 1];
            if (fullPath.size() == name.size() || lastChar == '/' || lastChar == '\\') {
                return entry.second;
            }
        }
    }
    return nullptr;
}


bool CreateDirectoryRecursive(const std::string &dirName, std::error_code &err) {
    err.clear();
    if (!std::filesystem::create_directories(dirName, err)) {
        if (std::filesystem::exists(dirName)) {
            err.clear();
            return true;
        }
        return false;
    }
    return true;
}

inline bool ValidateGlobals() {
    if (!loaded_arc_base) {
        Logger::error("No loaded archive! This is a bug!");
        return false;
    }
    if (!selected_entry) {
        Logger::error("No selected entry found! This is a bug!");
        return false;
    }
    if (!current_buffer) {
        Logger::error("current_buffer is not initialized! This is a bug!");
        return false;
    }
    return true;
}

bool VirtualArc::ExtractEntry(fs::path basePath, Entry *entry, fs::path outputPath) {
    if (!ValidateGlobals()) return false;

#ifdef linux
    std::replace(entry->name.begin(), entry->name.end(), '\\', '/');
#endif

    fs::path fullOutputPath = basePath / entry->name;

    std::error_code err;
    if (!CreateDirectoryRecursive(fullOutputPath.parent_path(), err)) {
        Logger::error("Failed to create directory: %s", err.message().c_str());
        return false;
    }

    u8 *extracted = loaded_arc_base->OpenStream(entry, current_buffer);

    FILE *file = fopen(fullOutputPath.c_str(), "wb");
    if (!file) return false;
    fwrite(extracted, sizeof(u8), entry->size, file);
    fclose(file);

    return true;
}

void VirtualArc::ExtractAll() {
    if (!ValidateGlobals()) return;

    auto entries = loaded_arc_base->GetEntries();
    std::string fileName = fs::path(rootNode->FileName).filename().string();

    std::string basePath = "extracted/" + fileName;
    fs::create_directories(basePath);

    for (auto &entry : entries) {
        if (!VirtualArc::ExtractEntry(basePath, entry.second)) {
            Logger::error("Failed to extract: %s", entry.second->name.c_str());
        }
    }
}

void VirtualArc::ExtractEntry(std::string path) {
    VirtualArc::ExtractEntry(path, selected_entry);
}

void DirectoryNode::UnloadSelectedFile() {
    if (preview_state.contents.size > 0) {
        preview_state.contents.data = nullptr;
    }

    Image::UnloadTexture(preview_state.texture.id);
    Image::UnloadAnimation(&preview_state.texture.anim);

    preview_state.texture.id = {};
    preview_state.content_type = PContentType::UNKNOWN;
    preview_state.contents = {};

    if (preview_state.audio.music) {
        Mix_FreeMusic(preview_state.audio.music);
        preview_state.audio.music = {};
    }
    preview_state.audio.buffer = {};
    preview_state.audio.playing = false;
    preview_state.audio.time = {};
    preview_state.audio.scrubberDragging = false;
    if (preview_state.audio.update_timer) {
        SDL_RemoveTimer(preview_state.audio.update_timer);
        preview_state.audio.update_timer = 0;
    }
    current_sound = nullptr;
}

void DirectoryNode::Unload(Node *node) {
    for (Node* child : node->Children) {
        Unload(child);
        delete child;
    }
    node->Children.clear();
}

inline void SortChildren(DirectoryNode::Node *node) {
    std::sort(node->Children.begin(), node->Children.end(),
    [](const DirectoryNode::Node* a, const DirectoryNode::Node* b) {
        if (a->IsDirectory != b->IsDirectory) return a->IsDirectory > b->IsDirectory;

        return Utils::ToLower(a->FileName) < Utils::ToLower(b->FileName);
    });
}

inline void SortChildrenBy(DirectoryNode::Node* node, auto func) {
    std::sort(node->Children.begin(), node->Children.end(), func);
}

bool DirectoryNode::AddNodes(Node *node, const fs::path &parentPath) {
    try {
        if (node->IsVirtualRoot) {
            auto entries = loaded_arc_base->GetEntries();

            for (const auto &entry : entries) {
                #ifdef linux
                std::replace(entry.second->name.begin(), entry.second->name.end(), '\\', '/');
                #endif

                fs::path entryPath(entry.second->name);
                Node *current = node;

                for (auto it = entryPath.begin(); it != entryPath.end(); ++it) {
                    std::string part = it->string();
                    bool isLast = (std::next(it) == entryPath.end());

                    auto found = std::find_if(current->Children.begin(), current->Children.end(),
                        [&part](const Node* child) {
                            return child->FileName == part;
                        }
                    );

                    if (found == current->Children.end()) {
                        Node *newNode = new Node{
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
                fs::path path = entry.path();
                std::string fileName = path.filename();
                // Why the hell does steam still have this? It's an intentionally broken symlink.
                // https://github.com/ValveSoftware/steam-for-linux/issues/5245
                if (fileName.contains(".steampath")) {
                    continue;
                }
                Node *childNode = new Node {
                    .FullPath = path.string(),
                    .FileName = fileName,
                    .FileSize = Utils::GetFileSize(path),
                    .FileSizeBytes = entry.is_directory() ? 0 : fs::file_size(entry),
                    .LastModified = Utils::GetLastModifiedTime(path),
                    .LastModifiedUnix = (u64)fs::last_write_time(entry).time_since_epoch().count(),
                    .Children = {},
                    .IsDirectory = entry.is_directory()
                };
                node->Children.push_back(childNode);
            }
        }
        SortChildren(node);
        return true;
    } catch (const fs::filesystem_error &e) {
        printf("Error accessing directory: %s\n", e.what());
        ui_error = UIError::CreateError(e.what(), "Error accessing directory!");

        return false;
    }
}

void UnloadArchive() {
    if (loaded_arc_base) {
        delete loaded_arc_base;
        loaded_arc_base = nullptr;
    }
    if (current_buffer) {
        free(current_buffer);
        current_buffer = nullptr;
    }
}

std::filesystem::path LinuxExpandUserPath(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return std::filesystem::path(path);
    }

    const char* home = std::getenv("HOME");
    if (path.size() == 1) {
        return std::filesystem::path(home);
    } else if (path[1] == '/') {
        return std::filesystem::path(home) / path.substr(2);
    }

    return "/";
}

DirectoryNode::Node *DirectoryNode::CreateTreeFromPath(const std::string& rootPath, Node *parent) {
    bool is_dir = fs::is_directory(rootPath);
    Node *newRootNode = new Node {
        .FullPath = rootPath,
        .FileName = rootPath,
        .FileSize = Utils::GetFileSize(rootPath),
        .FileSizeBytes = is_dir ? 0 : fs::file_size(rootPath),
        .LastModified = Utils::GetLastModifiedTime(rootPath),
        .LastModifiedUnix = (u64)fs::last_write_time(rootPath).time_since_epoch().count(),
        .Parent = parent,
        .Children = {},
        .IsDirectory = is_dir,
        .IsVirtualRoot = !is_dir,
    };

    AddNodes(newRootNode, rootPath);

    return newRootNode;
}

void DirectoryNode::ReloadRootNode(Node *node) {
    if (fs::is_directory(node->FullPath)) {
        rootNode = CreateTreeFromPath(fs::canonical(node->FullPath).string());
    }
}

Uint32 TimerUpdateCB(void* userdata, Uint32 interval, Uint32 param) {
    if (current_sound) {
        double current_time = Mix_GetMusicPosition(current_sound);
        if (!preview_state.audio.scrubberDragging) {
            preview_state.audio.time.current_time_min = current_time / 60;
            preview_state.audio.time.current_time_sec = fmod(current_time, 60);
        }
    }
    return interval;
}

void DirectoryNode::HandleFileClick(Node *node) {
    std::string filename = node->FileName;
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    bool isVirtualRoot = rootNode->IsVirtualRoot;
    u8* entry_buffer = nullptr;
    size_t size = 0;

    if (isVirtualRoot && !node->IsDirectory) {
        selected_entry = FindEntryByNode(loaded_arc_base->GetEntries(), node);

        if (selected_entry) {
            if (current_buffer) {
                auto arc_read = loaded_arc_base->OpenStream(selected_entry, current_buffer);
                size = selected_entry->size;
                entry_buffer = malloc<u8>(size);
                memcpy(entry_buffer, arc_read, size);
            } else {
                Logger::error("current_buffer is not initialized!");
                return;
            }
        }
    } else {
        auto [fs_buffer, fs_size] = read_file_to_buffer<u8>(node->FullPath.c_str());
        size = fs_size;
        entry_buffer = malloc<u8>(size);
        memcpy(entry_buffer, fs_buffer, size);
        free(fs_buffer);
    }

    if (entry_buffer == nullptr) {
        Logger::error("Unable to resolve what entry_buffer should be! Aborting.");
        return;
    }

    auto format = extractor_manager.getExtractorFor(entry_buffer, size, ext);

    if (format == nullptr) {
        UnloadSelectedFile();

        preview_state.contents = {
            .data = current_buffer,
            .size = size,
            .path = node->FullPath,
            .ext = ext,
            .fileName = node->FileName
        };

        if (Image::IsImageExtension(ext)) {
            Image::LoadImage(entry_buffer, size, &preview_state.texture.id, preview_state.texture.size);
            if (*preview_state.texture.size.x < 256) {
                Image::LoadImage(entry_buffer, size, &preview_state.texture.id, preview_state.texture.size, GL_NEAREST);
            }
            preview_state.content_type = IMAGE;
            free(entry_buffer);
        } else if (Image::IsGif(ext)) {
            Image::LoadGifAnimation(entry_buffer, size, &preview_state.texture.anim);
            preview_state.content_type = GIF;
            preview_state.texture.frame = 0;
            preview_state.texture.last_frame_time = SDL_GetTicks();
            free(entry_buffer);
        } else if (Audio::IsAudio(ext)) {
            if (isVirtualRoot) {
                preview_state.audio.buffer = entry_buffer;
                SDL_IOStream *snd_io = SDL_IOFromConstMem(preview_state.audio.buffer, size);
                current_sound = Mix_LoadMUS_IO(snd_io, true);
                if (!current_sound) {
                    Logger::error("Failed to load audio: %s", SDL_GetError());
                    free(entry_buffer);
                    preview_state.audio.buffer = nullptr;
                }
            } else {
                current_sound = Mix_LoadMUS(node->FullPath.c_str());
                free(entry_buffer);
            }

            if (current_sound) {
                preview_state.content_type = AUDIO;
                Mix_PlayMusic(current_sound, 1);
                double duration = Mix_MusicDuration(current_sound);
                preview_state.audio.music = current_sound;
                preview_state.audio.playing = true;
                preview_state.audio.time.total_time_min = duration / 60;
                preview_state.audio.time.total_time_sec = fmod(duration, 60.0);
                preview_state.audio.update_timer = SDL_AddTimer(1000, TimerUpdateCB, nullptr);
            }
        } else if (ElfFile::IsValid(entry_buffer)) {
            auto *elfFile = new ElfFile(entry_buffer, size);
            preview_state.contents.elfFile = elfFile;
            preview_state.content_type = ELF;
        } else {
            auto text = std::string((char*)entry_buffer, size);
            if (text.size() >= 2 && text[0] == '\xFF' && text[1] == '\xFE') {
                std::u16string utf16((char16_t*)text.data() + 1, (text.size() - 1) / 2);
                editor.SetText(TextConverter::UTF16ToUTF8(utf16));
            } else {
                editor.SetText(text);
            }
            editor.SetTextChanged(false);
            editor.SetColorizerEnable(size <= 200000);
            editor.SetShowWhitespaces(false);
            free(entry_buffer);
        }
        return;
    }

    auto arc = format->TryOpen(entry_buffer, size, node->FileName);
    if (arc == nullptr) {
        Logger::error("Failed to open archive: %s! Attempted to open as: %s", node->FileName.c_str(), format->GetTag().c_str());
        free(entry_buffer);
        return;
    }

    if (loaded_arc_base) {
        delete loaded_arc_base;
    }
    loaded_arc_base = arc;

    if (current_buffer) {
        free(current_buffer);
    }

    current_buffer = malloc<u8>(size);
    memcpy(current_buffer, entry_buffer, size);
    free(entry_buffer);

    rootNode = CreateTreeFromPath(node->FullPath);
}

bool CanReadDirectory(const std::string& path) {
    return access(path.c_str(), R_OK | X_OK) == 0;
}

void DirectoryNode::Display(Node *node) {
    ImGui::TableNextRow();
    ImGui::PushID(node);

    ImGui::TableNextColumn();
    ImGui::Selectable(node->FileName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);

    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node->IsDirectory) {
            if (!rootNode->IsVirtualRoot && !CanReadDirectory(node->FullPath)) {
                Logger::error("Cannot access directory: %s", node->FullPath.c_str());
                ui_error = UIError::CreateError("You do not have permission to access this directory!", "Access Denied");
            } else {
                if (rootNode->IsVirtualRoot) {
                    node->IsVirtualRoot = true;
                    node->Parent = rootNode;
                    rootNode = node;
                } else {
                    rootNode = CreateTreeFromPath(node->FullPath, rootNode);
                }
            }
        } else {
            HandleFileClick(node);
        }
        if (rootNode->FullPath.ends_with("/")) {
            SetFilePath(rootNode->FullPath);
        } else {
            SetFilePath(rootNode->FullPath + "/");
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

static char *file_path_buf = (char*)calloc(sizeof(char), 1024);

void SetFilePath(const std::string& file_path) {
    std::string normalized = file_path.ends_with("/") ? file_path : (file_path + "/");
    std::strncpy(file_path_buf, normalized.c_str(), 1024);
    file_path_buf[1023] = '\0';
}

#define FB_COLUMNS 3
void DirectoryNode::Setup(Node *node) {
    ImGui::PushID(node);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 48);
    if (ImGui::InputText("##file_path", file_path_buf, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string expanded_path = LinuxExpandUserPath(std::string(file_path_buf));
        SetFilePath(expanded_path);
        rootNode = CreateTreeFromPath(expanded_path);
    }

    ImGui::SameLine();

    InfoDialog();

    if (ImGui::Button(HELP_ICON, {40, 0})) {
        ImGui::OpenPopup("Format Info");
    };

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
                SortChildrenBy(node, [](const Node* a, const Node* b) {
                    return a->FileSizeBytes > b->FileSizeBytes;
                });
            } else if (strcmp(name, "Last Modified") == 0) {
                SortChildrenBy(node, [](const Node* a, const Node* b) {
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
    AddDirectoryNodeChild("..", [&node](){
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (node->Parent) {
                rootNode = node->Parent;
            } else {
                UnloadArchive();
                DirectoryNode::Unload(rootNode);
                if (node->FullPath.ends_with("/")) {
                    node->FullPath.pop_back();
                }
                auto parent_path = fs::path(node->FullPath).parent_path();
                rootNode = DirectoryNode::CreateTreeFromPath(parent_path.string());
                if (current_buffer) {
                    free(current_buffer);
                    current_buffer = nullptr;
                }
            }
            SetFilePath(rootNode->FullPath);
        }
    });
    ImGui::TableNextColumn();

    for (auto childNode : node->Children) {
        Display(childNode);
    }

    ImGui::EndTable();
    ImGui::PopID();
}
