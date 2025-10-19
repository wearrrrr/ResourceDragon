#define _CRT_SECURE_NO_WARNINGS
#include <Audio.h>
#include <DirectoryNode.h>
#include <Image.h>
#include <algorithm>
#include <cmath>
#include <csignal>
#include <filesystem>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <util/Text.h>
#include <util/int.h>
#include <Utils.h>

#include "SDL3_mixer/SDL_mixer.h"
#include <UIError.h>
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
    void Text(std::string_view str) {
        ImGui::Text("%s", str.data());
    };
}

inline void TableTextCentered(const char* label) {
    float textHeight = ImGui::GetTextLineHeight();
    float frameHeight = ImGui::GetFrameHeight();
    float y_offset = (frameHeight - textHeight) * 0.5f;

    float frameWidth = ImGui::GetContentRegionAvail().x;
    float x_offset = (frameWidth - textHeight) * 0.5f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x_offset);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y_offset);
    ImGui::TextUnformatted(label);
}

static bool settings_open = false;

struct FileLoadingResult {
    bool success = false;
    u8* entry_buffer = nullptr;
    size_t size = 0;
    std::string_view error_message;
    DirectoryNode::Node* node = nullptr;
    std::string ext;
    bool isVirtualRoot = false;
    Entry* selected_entry = nullptr;
    ContentType typeOverride = ContentType::UNKNOWN;
    usize tab_index = 0;
};

static std::atomic<bool> file_loading_in_progress = false;
static std::mutex file_loading_mutex;
static std::queue<FileLoadingResult> completed_loads;

void InfoDialog() {
    ImGui::SetNextWindowSize({ImGui::GetIO().DisplaySize.x / 2, 425.0f});
    if (ImGui::BeginPopupModal("Settings", &settings_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
            ImGui::SetWindowFocus(nullptr);
        }
        if (ImGui::BeginTabBar("SettingsBar")) {
            if (ImGui::BeginTabItem("Formats")) {
                if (ImGui::BeginTable("FormatsTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("Name", 200.0f);
                    ImGui::TableSetupColumn("Description", 400.0f);
                    ImGui::TableHeadersRow();

                    for (const auto &[name, format] : extractor_manager->GetFormats()) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text(name);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", format->GetDescription());
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                if (ImGui::BeginTable("SettingsTable", 2)) {
                    ImGui::TableSetupColumn("Name", 200.0f);
                    ImGui::TableSetupColumn("Value", 300.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    TableTextCentered("Theme");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Combo("###ThemeCombo", &theme_manager.currentTheme, theme_manager.themes.data(), theme_manager.themes.size())) {
                        theme_manager.SetTheme(theme_manager.GetCurrentTheme());
                    };

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    TableTextCentered("Default to Hex View");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Checkbox("###DefaultToHexViewCheckbox", &default_to_hex_view)) {
                        text_viewer_override = !default_to_hex_view;
                    };

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndPopup();
    }
}


Entry* FindEntryByNode(const EntryMapPtr &entries, const DirectoryNode::Node *node) {
    const std::string &fullPath = node->FullPath;
    for (const auto &[_, entry] : entries) {
        const std::string &name = entry->name;

        if (fullPath.size() >= name.size() && fullPath.compare(fullPath.size() - name.size(), name.size(), name) == 0) {
            u8 lastChar = fullPath[fullPath.size() - name.size() - 1];
            if (fullPath.size() == name.size() || lastChar == '/' || lastChar == '\\') {
                return entry;
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

#if defined(__linux__) || defined(EMSCRIPTEN)
    std::replace(entry->name.begin(), entry->name.end(), '\\', '/');
#endif

    fs::path fullOutputPath = basePath / entry->name;

    std::error_code err;
    if (!CreateDirectoryRecursive(fullOutputPath.parent_path().string(), err)) {
        Logger::error("Failed to create directory: {}", err.message().data());
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

    for (auto &[_, entry] : entries) {
        if (!VirtualArc::ExtractEntry(basePath, entry)) {
            Logger::error("Failed to extract: {}", entry->name.data());
        }
    }
}

void VirtualArc::ExtractEntry(std::string path) {
    VirtualArc::ExtractEntry(path, selected_entry);
}

void DirectoryNode::UnloadSelectedFile() {
    text_editor__unsaved_changes = false;

    PreviewWinState &state = GetPreviewState(preview_index);

    if (default_to_hex_view) {
        state.contents.type = HEX;
        text_viewer_override = false;
    }

    if (state.contents.size > 0) {
        state.contents.data = nullptr;
    };

    state.contents.encoding = UTF8;

    Image::UnloadTexture(state.texture.id);
    Image::UnloadAnimation(&state.texture.anim);

    state.texture = {};
    image_preview.zoom = 1.0f;
    image_preview.pan = {0.0f, 0.0f};
    state.contents = {};
    state.contents.type = ContentType::UNKNOWN;

    state.audio.playing = false;
    if (state.audio.music) {
        MIX_StopAllTracks(state.audio.mixer, 0);
        MIX_DestroyAudio(state.audio.music);
        state.audio.music = nullptr;
    }
    if (state.audio.buffer) {
        free(state.audio.buffer);
        state.audio.buffer = nullptr;
    };
    state.audio.time = {};
    state.audio.scrubberDragging = false;
    if (state.audio.update_timer) {
        SDL_RemoveTimer(state.audio.update_timer);
        state.audio.update_timer = 0;
    }
    state.audio.music = nullptr;
}

void DirectoryNode::Unload(Node *node) {
    for (Node *child : node->Children) {
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

        if (sortAscending) return nameA < nameB;
        else return nameA > nameB;
    });
}

inline void DirectoryNode::SortChildrenBy(Node *node, auto func) {
    std::sort(node->Children.begin(), node->Children.end(), func);
}

bool DirectoryNode::AddNodes(Node *node, const fs::path &parentPath) {
#ifndef _WIN32
    try {
#endif
        if (node->IsVirtualRoot) {
            auto entries = loaded_arc_base->GetEntries();

            for (const auto& [_, entry] : entries) {
                // Still not entirely sure if this is necessary?
#if defined(__linux__) || defined(EMSCRIPTEN)
                std::replace(entry->name.begin(), entry->name.end(), '\\', '/');
#endif
                fs::path entryPath(entry->name);
                Node *current = node;

                for (auto it = entryPath.begin(); it != entryPath.end(); ++it) {
                    std::string part = it->string();
                    bool isLast = std::next(it) == entryPath.end();

                    auto found = std::find_if(current->Children.begin(), current->Children.end(),
                        [&part](const Node* child) {
                            return child->FileName == part;
                        }
                    );

                    if (found == current->Children.end()) {
                        const std::string fullPath = current->FullPath.empty() ? part : current->FullPath + (char)fs::path::preferred_separator + part;
                        Node *newNode = new Node {
                            .FullPath = fullPath,
                            .FileName = part,
                            .FileSize = isLast ? Utils::GetFileSize(entry->size) : "--",
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
            for (const auto &entry : fs::directory_iterator(parentPath)) {
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
#ifndef _WIN32
    } catch (const fs::filesystem_error &err) {
        SortChildrenAlphabetical(node, true);
        return false;
    }
#endif
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
#ifndef _WIN32
    try {
#endif
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
#ifndef _WIN32
    } catch (fs::filesystem_error err) {
        return rootNode;
    }
#endif

}

void DirectoryNode::ReloadRootNode(Node *node) {
    if (fs::is_directory(node->FullPath)) {
        rootNode = CreateTreeFromPath(fs::canonical(node->FullPath).string());
    }
}

Uint32 TimerUpdateCB(void* userdata, Uint32 interval, Uint32 param) {
    PreviewWinState &state = GetPreviewState(preview_index);
    if (state.audio.music) {
        double current_time = MIX_GetTrackPlaybackPosition(state.audio.track);
        double current_time_sec = MIX_FramesToMS(state.audio.spec.freq, current_time) / 1000.0f;
        if (!state.audio.scrubberDragging) {
            state.audio.time.current_time_min = current_time_sec / 60;
            state.audio.time.current_time_sec = fmod(current_time_sec, 60);
        }
    }
    return interval;
}

void InitializePreviewData(DirectoryNode::Node *node, u8 *entry_buffer, u64 size, const std::string &ext, bool isVirtualRoot, ContentType typeOverride = ContentType::UNKNOWN) {
    ContentType type;

    PreviewWinState &state = GetPreviewState(preview_index);

    if (typeOverride != ContentType::UNKNOWN) {
        type = typeOverride;
        state.contents.type = type;

        if (type == IMAGE) {
            Image::LoadImage(entry_buffer, size, &state.texture.id, state.texture.size);
            if (*state.texture.size.x < 256) {
                Image::LoadImage(entry_buffer, size, &state.texture.id, state.texture.size, GL_NEAREST);
            }
        } else if (type == GIF) {
            Image::LoadGifAnimation(entry_buffer, size, &state.texture.anim);
            state.texture.frame = 0;
            state.texture.last_frame_time = SDL_GetTicks();
        } else if (type == AUDIO) {
            if (isVirtualRoot) {
                state.audio.buffer = (u8*)malloc(size);
                memcpy(state.audio.buffer, entry_buffer, size);
                SDL_IOStream *snd_io = SDL_IOFromMem(state.audio.buffer, size);
                state.audio.music = MIX_LoadAudio_IO(state.audio.mixer, snd_io, true, true);
                if (!state.audio.music) {
                    Logger::error("Failed to load audio: {}", SDL_GetError());
                    state.audio.buffer = nullptr;
                }
            } else {
                state.audio.music = MIX_LoadAudio(state.audio.mixer, node->FullPath.data(), true);
            }

            if (state.audio.music) {
                MIX_SetTrackAudio(state.audio.track, state.audio.music);
                MIX_PlayTrack(state.audio.track, 1);
                if (!MIX_GetAudioFormat(state.audio.music, &state.audio.spec)) {
                    Logger::error("Failed to get audio format: {}", SDL_GetError());
                    MIX_DestroyAudio(state.audio.music);
                }
                double duration_frames = MIX_GetAudioDuration(state.audio.music);
                double duration_sec = MIX_FramesToMS(state.audio.spec.freq, duration_frames) / 1000.0;
                state.audio.playing = true;
                state.audio.time.total_time_min = duration_sec / 60;
                state.audio.time.total_time_sec = fmod(duration_sec, 60.0);
                state.audio.update_timer = SDL_AddTimer(1000, TimerUpdateCB, nullptr);
            }
        } else if (type == ELF) {
            auto *elfFile = new ElfFile(entry_buffer, size);
            state.contents.elfFile = elfFile;
        } else if (type == TEXT || type == HEX) {
            auto text = std::string((char*)entry_buffer, size);
            editor.SetText(text);
            editor.SetTextChanged(false);
        }

        return;
    }

    // Automatic detection
    if (Image::IsImageExtension(ext)) {
        Image::LoadImage(entry_buffer, size, &state.texture.id, state.texture.size);
        if (*state.texture.size.x < 256) {
            Image::LoadImage(entry_buffer, size, &state.texture.id, state.texture.size, GL_NEAREST);
        }
        type = IMAGE;
        state.contents.type = type;
    } else if (Image::IsGif(ext)) {
        Image::LoadGifAnimation(entry_buffer, size, &state.texture.anim);
        state.contents.type = GIF;
        state.texture.frame = 0;
        state.texture.last_frame_time = SDL_GetTicks();
    } else if (Audio::IsAudio(ext)) {
        type = AUDIO;
        state.contents.type = type;
        if (isVirtualRoot) {
            state.audio.buffer = (u8*)malloc(size);
            memcpy(state.audio.buffer, entry_buffer, size);
            SDL_IOStream *snd_io = SDL_IOFromMem(state.audio.buffer, size);
            state.audio.music = MIX_LoadAudio_IO(state.audio.mixer, snd_io, true, true);
            if (!state.audio.music) {
                Logger::error("Failed to load audio: {}", SDL_GetError());
                state.audio.buffer = nullptr;
            }
        } else {
            state.audio.music = MIX_LoadAudio(state.audio.mixer, node->FullPath.data(), true);
        }

        if (state.audio.music) {
            MIX_SetTrackAudio(state.audio.track, state.audio.music);
            MIX_PlayTrack(state.audio.track, 1);
            if (!MIX_GetAudioFormat(state.audio.music, &state.audio.spec)) {
                Logger::error("Failed to get audio format: {}", SDL_GetError());
                MIX_DestroyAudio(state.audio.music);
            }
            double duration_frames = MIX_GetAudioDuration(state.audio.music);
            double duration_sec = MIX_FramesToMS(state.audio.spec.freq, duration_frames) / 1000.0;
            state.audio.playing = true;
            state.audio.time.total_time_min = duration_sec / 60;
            state.audio.time.total_time_sec = fmod(duration_sec, 60.0);
            state.audio.update_timer = SDL_AddTimer(1000, TimerUpdateCB, nullptr);
        }
    } else if (ElfFile::IsValid(entry_buffer)) {
        auto *elfFile = new ElfFile(entry_buffer, size);
        state.contents.elfFile = elfFile;
        state.contents.type = ELF;
    } else {
        // Default to text if size is less than 3MB
        auto text = std::string((char*)entry_buffer, size);
        editor.SetText(text);
        editor.SetTextChanged(false);
        if (size < 3000000) {
            state.contents.type = TEXT;
        } else {
            state.contents.type = HEX;
        }
    }

    return;
}

void ProcessFileLoadingResult(const FileLoadingResult& result, ContentType typeOverride = ContentType::UNKNOWN) {
    PreviewWinState &state = GetPreviewState(result.tab_index);
    if (!result.success) {
        if (!result.error_message.empty()) {
            ui_error = UIError::CreateError(result.error_message.data(), "Failed to open file!");
            Logger::error("File loading failed: {}", result.error_message.data());
        }

        if (result.entry_buffer) {
            free(result.entry_buffer);
        }
        return;
    }

    if (result.isVirtualRoot && result.selected_entry) {
        selected_entry = result.selected_entry;
    }

    if (typeOverride != ContentType::UNKNOWN) {
        state.contents = {
            .data = result.entry_buffer,
            .size = result.size,
            .path = result.node->FullPath,
            .ext = result.ext,
            .fileName = result.node->FileName
        };

        InitializePreviewData(result.node, result.entry_buffer, result.size, result.ext, result.isVirtualRoot, typeOverride);
        return;
    }

    auto format_list = extractor_manager->GetExtractorCandidates(result.entry_buffer, result.size, result.ext);

    if (format_list.size() <= 0) {
        // This is a regular file preview (not an archive)
        state.contents = {
            .data = result.entry_buffer,
            .size = result.size,
            .path = result.node->FullPath,
            .ext = result.ext,
            .fileName = result.node->FileName
        };

        InitializePreviewData(result.node, result.entry_buffer, result.size, result.ext, result.isVirtualRoot, typeOverride);
        return;
    } else if (format_list.size() > 1) {
        Logger::warn("Multiple formats found for {}", result.node->FileName.data());
        Logger::warn("All formats detected as compatible: ");
        for (auto& format : format_list) {
            printf("\t%s\n", format->GetTag());
        }
    }

    auto format = format_list[0];
    auto arc = format->TryOpen(result.entry_buffer, result.size, result.node->FileName);
    if (arc == nullptr) {
        Logger::error("Failed to open archive: {}! Attempted to open as: {}", result.node->FileName.data(), format->GetTag());
        char message_buffer[512];
        snprintf(message_buffer, sizeof(message_buffer), "Failed to open archive: '%s'!\nAttempted to open as %s\n", result.node->FileName.data(), format->GetTag());
        ui_error = UIError::CreateError(message_buffer, "Failed to open archive!");
        free(result.entry_buffer);
        return;
    }

    // Clean up previous archive if it exists
    if (loaded_arc_base) {
        delete loaded_arc_base;
        loaded_arc_base = nullptr;
    }
    loaded_arc_base = arc;

    if (current_buffer) {
        free(current_buffer);
        current_buffer = nullptr;
    }

    current_buffer = malloc<u8>(result.size);
    if (current_buffer) {
        memcpy(current_buffer, result.entry_buffer, result.size);
    } else {
        Logger::error("Failed to allocate current_buffer for archive");
    }

    free(result.entry_buffer);

    rootNode = DirectoryNode::CreateTreeFromPath(result.node->FullPath);
}

void DirectoryNode::HandleFileClick(Node *node, ContentType typeOverride, usize tab_index) {
    if (file_loading_in_progress.load()) {
        Logger::warn("File loading already in progress, ignoring request for {}", node->FileName.c_str());
        return;
    }

    preview_index = tab_index;
    UnloadSelectedFile();

    std::string filename = node->FileName;
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    bool isVirtualRoot = rootNode->IsVirtualRoot;

    fb__loading_arc = true;
    fb__loading_file_name = filename;
    file_loading_in_progress = true;

    std::thread loading_thread([node, ext, isVirtualRoot, typeOverride, tab_index]() {
        FileLoadingResult result;
        result.node = node;
        result.ext = ext;
        result.isVirtualRoot = isVirtualRoot;
        result.typeOverride = typeOverride;
        result.tab_index = tab_index;

#ifndef _WIN32
        try {
#endif
            if (isVirtualRoot && !node->IsDirectory) {
                Entry* entry_to_process = nullptr;
                {
                    std::lock_guard<std::mutex> lock(file_loading_mutex);
                    entry_to_process = FindEntryByNode(loaded_arc_base->GetEntries(), node);
                }

                if (entry_to_process) {
                    if (current_buffer) {
                        auto arc_read = loaded_arc_base->OpenStream(entry_to_process, current_buffer);
                        if (arc_read == nullptr) {
                            result.error_message = "Received nullptr from OpenStream! Cannot show entry.";
                            result.success = false;
                            Logger::error("OpenStream failed for entry: {}", node->FileName.c_str());
                        } else {
                            result.size = entry_to_process->size;
                            result.entry_buffer = malloc<u8>(result.size);
                            if (result.entry_buffer) {
                                memcpy(result.entry_buffer, arc_read, result.size);
                                free(arc_read);
                                result.success = true;
                                result.selected_entry = entry_to_process;
                            } else {
                                result.error_message = "Failed to allocate memory for entry buffer";
                                result.success = false;
                                free(arc_read);
                                Logger::error("Memory allocation failed for entry: {}", node->FileName.c_str());
                            }
                        }
                    } else {
                        result.error_message = "current_buffer is not initialized!";
                        result.success = false;
                        Logger::error("current_buffer is not initialized for entry: {}", node->FileName.c_str());
                    }
                } else {
                    result.error_message = "Entry not found in archive";
                    result.success = false;
                    Logger::error("Entry not found in archive: {}", node->FileName.c_str());
                }
            } else {
                auto [fs_buffer, fs_size] = read_file_to_buffer<u8>(node->FullPath.data());
                if (!fs_buffer) {
                    char error_message[512];
                    snprintf(error_message, sizeof(error_message), "Failed to read file from filesystem: %s", node->FullPath.c_str());
                    result.error_message = error_message;
                    result.success = false;
                    Logger::error("Failed to read file: {}", node->FullPath.c_str());
                } else {
                    result.size = fs_size;
                    result.entry_buffer = malloc<u8>(result.size);
                    if (result.entry_buffer) {
                        memcpy(result.entry_buffer, fs_buffer, result.size);
                        free(fs_buffer);
                        result.success = true;
                    } else {
                        result.error_message = "Failed to allocate memory for file buffer";
                        result.success = false;
                        free(fs_buffer);
                        Logger::error("Memory allocation failed for file: {}", node->FileName.c_str());
                    }
                }
            }
#ifndef _WIN32
        } catch (const std::exception& e) {
            char error_message[512];
            snprintf(error_message, sizeof(error_message), "Exception during file loading: %s", e.what());
            result.error_message = error_message;
            result.success = false;
            Logger::error("Exception while loading {}: {}", node->FileName.c_str(), e.what());
        } catch (...) {
            result.error_message = "Unknown exception during file loading";
            result.success = false;
            Logger::error("Unknown exception while loading {}", node->FileName.c_str());
        }
#endif

        std::lock_guard<std::mutex> lock(file_loading_mutex);
        completed_loads.push(result);

        // Reset loading state
        fb__loading_arc = false;
        fb__loading_file_name = "";
        file_loading_in_progress = false;
    });

    loading_thread.detach();
}

void DirectoryNode::ProcessPendingFileLoads(ContentType typeOverride) {
    std::lock_guard<std::mutex> lock(file_loading_mutex);
    while (!completed_loads.empty()) {
        FileLoadingResult result = completed_loads.front();
        completed_loads.pop();
        // Use the result's typeOverride if it's set, otherwise use the parameter
        ContentType effectiveTypeOverride = (result.typeOverride != ContentType::UNKNOWN) ? result.typeOverride : typeOverride;
        ProcessFileLoadingResult(result, effectiveTypeOverride);
    }
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
                Logger::error("Cannot access directory: {}", node->FullPath.data());
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
            HandleFileClick(node, ContentType::UNKNOWN, preview_index);
        }
        if (rootNode->FullPath.ends_with("/")) {
            SetFilePath(rootNode->FullPath);
        } else {
            SetFilePath(rootNode->FullPath + (char)fs::path::preferred_separator);
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
    char sep = fs::path::preferred_separator;

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
        settings_open = true;
        ImGui::OpenPopup("Settings");
    };

    ImGui::BeginTable("DirectoryTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentDisable);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 90.0f);
    ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 180.0f);
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
                if (a->IsDirectory != b->IsDirectory) {
                    return a->IsDirectory ? -1 : 1;
                }
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
        DirectoryNode::Display(childNode);
    }

    ImGui::EndTable();
    ImGui::PopID();
}
