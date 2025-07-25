#pragma once

// #define DEBUG

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <gl3.h>
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
    union {
      const Elf32_Header *elf32;
      const Elf64_Header *elf64;
    } elf_header;
    ElfFile *elfFile;
};



struct PreviewWinState {
  PWinStateContents contents;
  PWinStateAudio audio;
  PWinStateTexture texture;
};

inline PreviewWinState preview_state = {
    .audio {
        .volumePercent = 100
    }
};
inline ExtractorManager extractor_manager;

inline ArchiveBase *loaded_arc_base = nullptr;
inline u8 *current_buffer = nullptr;
inline Entry *selected_entry = nullptr;

inline TextEditor editor;

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
