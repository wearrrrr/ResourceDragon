#include <Audio.h>
#include <DirectoryNode.h>
#include <Image.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <functional>
#include <util/Text.h>
#include <util/int.h>
#include <Utils.h>

#include "SDL3_mixer/SDL_mixer.h"
#include "UIError.h"
#include <imgui.h>
#ifdef _WIN32
#ifndef __MINGW32__
#define NOMINMAX
#endif
#include <windows.h>
#endif
#if defined(__linux__) || defined(EMSCRIPTEN)
#include <unistd.h>
#endif
#include <gl3.h>
#include "state.h"

#include <stdio.h>

#include "icons.h"

namespace ImGui {
    void Text(const std::string_view &str) {
        ImGui::Text("%s", str.data());
    };
}

void InfoDialog() {
    if (ImGui::BeginPopupModal("Format Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {

        if (ImGui::BeginTable("FormatsTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 250.0f);
            ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthStretch, 250.0f);
            ImGui::TableHeadersRow();

            for (const auto &pair : extractor_manager->GetFormats()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(pair.first);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(pair.second->GetDescription());
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
    const std::string &fullPath = node->FullPath;
    for (const auto &entry : entries) {
        const std::string &name = entry.second->name;

        if (fullPath.size() >= name.size() && fullPath.compare(fullPath.size() - name.size(), name.size(), name) == 0) {
            u8 lastChar = fullPath[fullPath.size() - name.size() - 1];
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

bool VirtualArc::ExtractEntry(const fs::path &basePath, Entry *entry, fs::path outputPath) {
    if (!ValidateGlobals()) return false;

#ifdef __linux__
    std::replace(entry->name.begin(), entry->name.end(), '\\', '/');
#endif

    fs::path fullOutputPath = basePath / entry->name;

    std::error_code err;
    if (!CreateDirectoryRecursive(fullOutputPath.parent_path().string(), err)) {
        Logger::error("Failed to create directory: %s", err.message().data());
        return false;
    }

    u8 *extracted = loaded_arc_base->OpenStream(entry, current_buffer);

    FILE *file = fopen(fullOutputPath.string().c_str(), "wb");
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
            Logger::error("Failed to extract: %s", entry.second->name.data());
        }
    }
}

void VirtualArc::ExtractEntry(std::string path) {
    VirtualArc::ExtractEntry(path, selected_entry);
}

void DirectoryNode::UnloadSelectedFile() {
    text_editor__unsaved_changes = false;

    if (preview_state.contents.size > 0) {
        preview_state.contents.data = nullptr;
    };

    preview_state.contents.encoding = UTF8;

    Image::UnloadTexture(preview_state.texture.id);
    Image::UnloadAnimation(&preview_state.texture.anim);

    preview_state.texture = {};
    image_preview.zoom = 1.0f;
    image_preview.pan = {0.0f, 0.0f};
    preview_state.contents = {};
    preview_state.contents.type = ContentType::UNKNOWN;

    preview_state.audio.playing = false;
    if (preview_state.audio.music) {
        MIX_StopAllTracks(preview_state.audio.mixer, 0);
        MIX_DestroyAudio(preview_state.audio.music);
        preview_state.audio.music = nullptr;
    }
    if (preview_state.audio.buffer) {
        Logger::log("freeing audio buffer");
        free(preview_state.audio.buffer);
        preview_state.audio.buffer = nullptr;
    };
    preview_state.audio.time = {};
    preview_state.audio.scrubberDragging = false;
    if (preview_state.audio.update_timer) {
        SDL_RemoveTimer(preview_state.audio.update_timer);
        preview_state.audio.update_timer = 0;
    }
    preview_state.audio.music = nullptr;
}

void DirectoryNode::Unload(Node *node) {
    for (Node* child : node->Children) {
        Unload(child);
        delete child;
    }
    node->Children.clear();
}

inline void DirectoryNode::SortChildrenAlphabetical(Node *node, bool sortAscending) {
    std::sort(node->Children.begin(), node->Children.end(),
    [=](const Node* a, const Node* b) {
        if (a->IsDirectory != b->IsDirectory)
            return a->IsDirectory > b->IsDirectory;

        std::string nameA = Utils::ToLower(a->FileName);
        std::string nameB = Utils::ToLower(b->FileName);

        if (sortAscending)
            return nameA < nameB;
        else
            return nameA > nameB;
    });
}

inline void DirectoryNode::SortChildrenBy(Node *node, auto func) {
    std::sort(node->Children.begin(), node->Children.end(), func);
}

bool DirectoryNode::AddNodes(Node *node, const fs::path &parentPath) {
    try {
        if (node->IsVirtualRoot) {
            auto entries = loaded_arc_base->GetEntries();

            for (const auto &entry : entries) {
                // Still not entirely sure if this is necessary?
#ifdef __linux__
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
                        Node *newNode = new Node {
                            .FullPath = (current->FullPath.empty() ? part : current->FullPath + static_cast<char>(fs::path::preferred_separator) + part),
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
                std::string fileName = path.filename().string();

                Node *childNode = new Node {
                    .FullPath = path.string(),
                    .FileName = fileName,
                    .FileSize = Utils::GetFileSize(path),
                    .FileSizeBytes = entry.is_directory() ? 0 : fs::file_size(entry),
                    .LastModified = Utils::GetLastModifiedTime(path.string()),
                    .LastModifiedUnix = (u64)fs::last_write_time(entry).time_since_epoch().count(),
                    .Children = {},
                    .IsDirectory = entry.is_directory()
                };
                node->Children.push_back(childNode);
            }
        }
        SortChildrenAlphabetical(node, true);
        return true;
    } catch (const fs::filesystem_error &err) {
        SortChildrenAlphabetical(node, true);
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
    try {
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
    } catch (fs::filesystem_error err) {
        return rootNode;
    }

}

void DirectoryNode::ReloadRootNode(Node *node) {
    if (fs::is_directory(node->FullPath)) {
        rootNode = CreateTreeFromPath(fs::canonical(node->FullPath).string());
    }
}

Uint32 TimerUpdateCB(void* userdata, Uint32 interval, Uint32 param) {
    if (preview_state.audio.music) {
        double current_time = MIX_GetTrackPlaybackPosition(preview_state.audio.track);
        double current_time_sec = MIX_FramesToMS(preview_state.audio.spec.freq, current_time) / 1000.0f;
        if (!preview_state.audio.scrubberDragging) {
            preview_state.audio.time.current_time_min = current_time_sec / 60;
            preview_state.audio.time.current_time_sec = fmod(current_time_sec, 60);
        }
    }
    return interval;
}

void InitializePreviewData(DirectoryNode::Node *node, u8 *entry_buffer, u64 size, const std::string &ext, bool isVirtualRoot) {
    if (Image::IsImageExtension(ext)) {
        Image::LoadImage(entry_buffer, size, &preview_state.texture.id, preview_state.texture.size);
        if (*preview_state.texture.size.x < 256) {
            Image::LoadImage(entry_buffer, size, &preview_state.texture.id, preview_state.texture.size, GL_NEAREST);
        }
        preview_state.contents.type = IMAGE;
    } else if (Image::IsGif(ext)) {
        Image::LoadGifAnimation(entry_buffer, size, &preview_state.texture.anim);
        preview_state.contents.type = GIF;
        preview_state.texture.frame = 0;
        preview_state.texture.last_frame_time = SDL_GetTicks();
    } else if (Audio::IsAudio(ext)) {
        preview_state.contents.type = AUDIO;
        if (isVirtualRoot) {
            preview_state.audio.buffer = (u8*)malloc(size);
            memcpy(preview_state.audio.buffer, entry_buffer, size);
            SDL_IOStream *snd_io = SDL_IOFromMem(preview_state.audio.buffer, size);
            preview_state.audio.music = MIX_LoadAudio_IO(preview_state.audio.mixer, snd_io, true, true);
            if (!preview_state.audio.music) {
                Logger::error("Failed to load audio: %s", SDL_GetError());
                preview_state.audio.buffer = nullptr;
            }
        } else {
            preview_state.audio.music = MIX_LoadAudio(preview_state.audio.mixer, node->FullPath.data(), true);
        }

        if (preview_state.audio.music) {
            MIX_SetTrackAudio(preview_state.audio.track, preview_state.audio.music);
            MIX_PlayTrack(preview_state.audio.track, 1);
            if (!MIX_GetAudioFormat(preview_state.audio.music, &preview_state.audio.spec)) {
                Logger::error("Failed to get audio format: %s", SDL_GetError());
                MIX_DestroyAudio(preview_state.audio.music);
            } else {
                Logger::log("Loaded audio sample rate: %d Hz", preview_state.audio.spec.freq);
            }
            double duration_frames = MIX_GetAudioDuration(preview_state.audio.music);
            double duration_sec = MIX_FramesToMS(preview_state.audio.spec.freq, duration_frames) / 1000.0;
            preview_state.audio.playing = true;
            preview_state.audio.time.total_time_min = duration_sec / 60;
            preview_state.audio.time.total_time_sec = fmod(duration_sec, 60.0);
            preview_state.audio.update_timer = SDL_AddTimer(1000, TimerUpdateCB, nullptr);
        }
    } else if (ElfFile::IsValid(entry_buffer)) {
        auto *elfFile = new ElfFile(entry_buffer, size);
        preview_state.contents.elfFile = elfFile;
        preview_state.contents.type = ELF;
    } else {
        auto text = std::string((char*)entry_buffer, size);
        editor.SetText(text);
        editor.SetTextChanged(false);
        editor.SetColorizerEnable(false);
        editor.SetShowWhitespaces(false);
        preview_state.contents.type = UNKNOWN;
    }

    return;
}

void DirectoryNode::HandleFileClick(Node *node) {
    UnloadSelectedFile();

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
                if (arc_read == nullptr) {
                    Logger::error("Received nullptr from OpenStream! Cannot show entry.");
                    return;
                }
                size = selected_entry->size;
                entry_buffer = malloc<u8>(size);
                memcpy(entry_buffer, arc_read, size);
                free(arc_read);
            } else {
                Logger::error("current_buffer is not initialized!");
                return;
            }
        }
    } else {
        auto [fs_buffer, fs_size] = read_file_to_buffer<u8>(node->FullPath.data());
        if (!fs_buffer) {
            return;
        }
        size = fs_size;
        entry_buffer = malloc<u8>(size);
        memcpy(entry_buffer, fs_buffer, size);
        free(fs_buffer);
    }

    if (entry_buffer == nullptr) {
        Logger::error("Unable to resolve what entry_buffer should be! Aborting.");
        return;
    }

    auto format_list = extractor_manager->GetExtractorCandidates(entry_buffer, size, ext);

    if (format_list.size() <= 0) {
        preview_state.contents = {
            .data = entry_buffer,
            .size = size,
            .path = node->FullPath,
            .ext = ext,
            .fileName = node->FileName
        };

        InitializePreviewData(node, entry_buffer, size, ext, isVirtualRoot);

        return;
    } else if (format_list.size() > 1) {
        Logger::warn("Multiple formats found for %s", node->FileName.data());
        Logger::warn("All formats detected as compatible: ");
        for (auto& format : format_list) {
            printf("\t%s\n", format->GetTag().data());
        }
    }
    // TODO: Show a UI to ask the user what format they meant to select if there's somehow a collision
    auto format = format_list[0];

    auto arc = format->TryOpen(entry_buffer, size, node->FileName);
    if (arc == nullptr) {
        Logger::error("Failed to open archive: %s! Attempted to open as: %s", node->FileName.data(), format->GetTag().data());
        char message_buffer[256];
        snprintf(message_buffer, sizeof(message_buffer), "Failed to open archive: '%s'!\nAttempted to open as %s\n", node->FileName.data(), format->GetTag().data());
        ui_error = UIError::CreateError(message_buffer, "Failed to open archive!");
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

inline bool CanReadDirectory(const std::string& path) {
    #if defined(__linux__) || defined(EMSCRIPTEN)
    return access(path.data(), R_OK | X_OK) == 0;
    #else
    // mhm yup go ahead bro
    return true;
    #endif
}

void DirectoryNode::Display(Node *node) {
    ImGui::TableNextRow();
    ImGui::PushID(node);

    ImGui::TableNextColumn();
    ImGui::Selectable(node->FileName.data(), false, ImGuiSelectableFlags_AllowDoubleClick);

    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (node->IsDirectory) {
            if (!rootNode->IsVirtualRoot && !CanReadDirectory(node->FullPath)) {
                Logger::error("Cannot access directory: %s", node->FullPath.data());
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
            SetFilePath(rootNode->FullPath + static_cast<char>(fs::path::preferred_separator));
        }

    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        fb__selectedItem = node;
        if (loaded_arc_base) {
            selected_entry = FindEntryByNode(loaded_arc_base->GetEntries(), node);
        }
        ImGui::OpenPopup("FBContextMenu");
    }

    ImGui::TableNextColumn();
    ImGui::Text(node->FileSize);

    ImGui::TableNextColumn();
    ImGui::Text(node->LastModified);

    ImGui::PopID();
}

using DNodeCallback = void(*)(void*);
struct Callback {
    void* context = nullptr;
    DNodeCallback call = nullptr;

    void operator()() const {
        if (call) call(context);
    }
};

void AddDirectoryNodeChild(std::string name, std::function<void()> cb = {}) {
    if (ImGui::Selectable(name.data(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        cb();
    };
}

static char *file_path_buf = (char*)calloc(sizeof(char), 1024);

void SetFilePath(const std::string& file_path) {
    char sep = static_cast<char>(fs::path::preferred_separator);

    bool ends_with_sep = false;
    if (!file_path.empty()) {
        char last_char = file_path.back();
        ends_with_sep = (last_char == '/') || (last_char == '\\');
    }

    std::string normalized = ends_with_sep ? file_path : (file_path + sep);

    size_t copy_len = std::min(normalized.size(), size_t(1023));
    std::memcpy(file_path_buf, normalized.data(), copy_len);
    file_path_buf[copy_len] = '\0';
}

#define FB_COLUMNS 3
void DirectoryNode::Setup(Node *node) {
    ImGui::PushID(node);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 48);
    if (ImGui::InputText("##file_path", file_path_buf, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
#if defined(__linux__) || defined(EMSCRIPTEN)
    std::string expanded_path = LinuxExpandUserPath(std::string(file_path_buf));
    SetFilePath(expanded_path);
    rootNode = CreateTreeFromPath(expanded_path);
#else
    // TODO: horrible for performance and probably does not work but I do not care right now :sob:
    SetFilePath(std::string(file_path_buf));
    rootNode = CreateTreeFromPath(std::string(file_path_buf));
#endif

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

    static bool sortAscending = true;
    static const char* currentSort = nullptr;

    for (int col = 0; col < FB_COLUMNS; col++) {
        ImGui::TableSetColumnIndex(col);
        const char* name = ImGui::TableGetColumnName(col);
        ImGui::PushID(col);
        if (ImGui::Selectable(name, false)) {
            if (currentSort == name) {
                sortAscending = !sortAscending;
            } else {
                currentSort = name;
                sortAscending = true;
            }

            auto folderFirst = [](const Node *a, const Node *b) {
                if (a->IsDirectory != b->IsDirectory)
                    return a->IsDirectory ? -1 : 1;
                // both are a folder
                return 0;
            };

            if (strcmp(currentSort, "Size") == 0) {
                SortChildrenBy(node, [&](const Node *a, const Node *b) {
                    int ff = folderFirst(a, b);
                    if (ff != 0) return ff < 0;

                    return sortAscending
                        ? a->FileSizeBytes < b->FileSizeBytes
                        : a->FileSizeBytes > b->FileSizeBytes;
                });
            }
            else if (strcmp(currentSort, "Last Modified") == 0) {
                SortChildrenBy(node, [&](const Node *a, const Node *b) {
                    int ff = folderFirst(a, b);
                    if (ff != 0) return ff < 0;

                    return sortAscending
                        ? a->LastModifiedUnix < b->LastModifiedUnix
                        : a->LastModifiedUnix > b->LastModifiedUnix;
                });
            }
            else {
                SortChildrenAlphabetical(node, sortAscending);
            }
        }
        ImGui::PopID();
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    AddDirectoryNodeChild("..", [node](){
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (node->Parent) {
                rootNode = node->Parent;
            } else {
                UnloadArchive();
                DirectoryNode::Unload(rootNode);
                if (node->FullPath.ends_with("/")) {
                    node->FullPath.pop_back();
                }
                auto parent_path = fs::path(node->FullPath).parent_path().string();
                if (parent_path.empty()) {
                    parent_path = "/";
                }
                rootNode = DirectoryNode::CreateTreeFromPath(parent_path);
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
