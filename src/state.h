#pragma once

#include "Themes.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3_mixer/SDL_mixer.h>
#ifdef _WIN32
#ifndef __MINGW32__
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <gl3.h>
#include <string>

#ifdef __linux__
#include <sys/inotify.h>
#define EVENT_SIZE sizeof(struct inotify_event)
#endif

#include "ArchiveFormats/ElfFile.h"
#include "GUI/Image.h"
#include "ExtractorManager.h"
#include "vec2.h"

#include <TextEditor/TextEditor.h>
#include <HexEditor/imgui_hex_editor.h>

struct PWinStateTexture {
  GLuint id;
  Vec2<int*> size;
  GifAnimation anim;
  int frame;
  int last_frame_time;
};

struct TimeInfo {
  int total_time_min;
  int total_time_sec;
  int current_time_min;
  int current_time_sec;
};

struct PWinStateAudio {
  MIX_Mixer *mixer;
  MIX_Audio *music;
  MIX_Track *track;
  SDL_AudioSpec spec;
  bool playing;
  u8 *buffer;
  int volumePercent;
  TimeInfo time;
  SDL_TimerID update_timer;
  bool shouldLoop;
  bool scrubberDragging;
};

enum ContentType {
    IMAGE,
    GIF,
    AUDIO,
    ELF,
    HEX,
    TEXT,
    UNKNOWN
};

enum ContentEncoding {
    UTF8,
    UTF16,
    SHIFT_JIS
};

struct PWinStateContents {
    u8 *data;
    size_t size;
    ContentType type;
    ContentEncoding encoding;
    std::string path;
    std::string ext;
    std::string fileName;
    bool reloadNeeded;
    union {
      const Elf32_Header *elf32;
      const Elf64_Header *elf64;
    } elf_header;
    ElfFile *elfFile;
};

struct PImageView {
    float zoom;
    ImVec2 pan;
};

struct PreviewWinState {
  PWinStateContents contents;
  PWinStateAudio audio;
  PWinStateTexture texture;
};

extern std::map<std::string, ImFont*> font_registry;

extern ThemeManager theme_manager;

extern PreviewWinState preview_state;
extern ExtractorManager *extractor_manager;

extern ArchiveBase *loaded_arc_base;
extern u8 *current_buffer;
extern Entry *selected_entry;

inline TextEditor editor;
inline MemoryEditor hex_editor;

extern PImageView image_preview;

extern bool text_editor__unsaved_changes;

#ifdef __linux__
extern int inotify_fd;
extern int inotify_wd;
#endif

template <typename T>
inline T* malloc(size_t size) {
    return (T*)malloc(size);
}

extern bool openDelPopup;
extern bool quitDialog;
