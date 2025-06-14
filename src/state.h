#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include "gl3.h"
#include <string>

#ifdef linux
#include <sys/inotify.h>
#define EVENT_SIZE sizeof(struct inotify_event)
#endif

#include "ArchiveFormats/ElfFile.h"
#include "GUI/Image.h"
#include "ExtractorManager.h"
#include "vec2.h"

#include <TextEditor/TextEditor.h>

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
  Mix_Music *music;
  bool playing;
  uint8_t *buffer;
  int volumePercent;
  TimeInfo time;
  SDL_TimerID update_timer;
  bool shouldLoop;
  bool scrubberDragging;
};

struct PWinStateContents {
    uint8_t *data;
    size_t size;
    std::string path;
    std::string ext;
    std::string fileName;
    union {
      const Elf32_Header *elf32;
      const Elf64_Header *elf64;
    } elf_header;
    ElfFile *elfFile;
};

enum PContentType {
    IMAGE,
    GIF,
    AUDIO,
    ELF,
    UNKNOWN
};

struct PreviewWinState {
  PContentType content_type;
  PWinStateContents contents;
  PWinStateAudio audio;
  PWinStateTexture texture;
};

struct UIError {
  std::string message;
  std::string title;
  bool show = false;
};

inline PreviewWinState preview_state = {
    .content_type = PContentType::UNKNOWN,
    .contents = {
        .data = nullptr,
        .size = 0,
        .path = "",
        .ext = "",
        .fileName = "",
        .elf_header = {},
        .elfFile = nullptr,
    },
    .audio = {
        .music = nullptr,
        .playing = false,
        .buffer = nullptr,
        .volumePercent = 100,
        .time = {
            .total_time_min = 0,
            .total_time_sec = 0,
            .current_time_min = 0,
            .current_time_sec = 0,
        },
        .update_timer = 0,
        .shouldLoop = false,
        .scrubberDragging = false,
      },
    .texture = {
        .id = 0,
        .size = {},
        .anim = {},
        .frame = 0,
        .last_frame_time = 0,
    }
};
inline ExtractorManager extractor_manager;

inline ArchiveBase *loaded_arc_base = nullptr;
inline uint8_t *current_buffer = nullptr;
inline Entry *selected_entry = nullptr;

inline TextEditor editor;

inline UIError ui_error = {
  .message = "",
  .title = "",
  .show = false
};

inline float img_preview__zoom = 1.0f;
inline ImVec2 img_preview__pan = {0.0f, 0.0f};

inline bool text_editor__unsaved_changes = false;

#ifdef linux
inline int inotify_fd;
inline int inotify_wd;
#endif

template <typename T>
T *malloc(size_t size) {
    return (T*)malloc(size);
}
