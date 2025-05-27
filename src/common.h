#pragma once

#include <SDL3/SDL.h>
#include <string>

#ifdef linux
#include <sys/inotify.h>
#define EVENT_SIZE (sizeof(struct inotify_event))
#endif

#include "imgui.h" // IWYU pragma: keep
#include "imgui_impl_opengl3.h" // IWYU pragma: keep
#include "imgui_impl_sdl3.h" // IWYU pragma: keep

#include "ArchiveFormats/ElfFile.h"
#include "ArchiveFormats/HSP/hsp.h" // IWYU pragma: keep
#include "ArchiveFormats/PFS/pfs.h" // IWYU pragma: keep
#include "ArchiveFormats/NitroPlus/nitroplus.h" // IWYU pragma: keep
#include "ArchiveFormats/SonicAdv/sonicadv.h" // IWYU pragma: keep
#include "ArchiveFormats/Touhou/thdat.h" // IWYU pragma: keep
#include "ArchiveFormats/XP3/xp3.h" // IWYU pragma: keep
#include "ExtractorManager.h"

#include "GUI/Audio.h" // IWYU pragma: keep
#include "GUI/Clipboard.h" // IWYU pragma: keep
#include "GUI/DirectoryNode.h" // IWYU pragma: keep
#include "GUI/Image.h" // IWYU pragma: keep
#include "GUI/TextEditor/TextEditor.h" // IWYU pragma: keep
#include "GUI/Theme/Themes.h" // IWYU pragma: keep
#include "GUI/Utils.h" // IWYU pragma: keep

#include "util/Logger.h" // IWYU pragma: keep
#include "util/Text.h" // IWYU pragma: keep

#include "zero_templates.h" // IWYU pragma: keep

struct PWinStateTexture {
  GLuint id;
  struct {
    int x;
    int y;
  } size;
  struct GifAnimation anim;
  int frame;
  int last_frame_time;
  bool firstFrame;
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
  unsigned char *buffer;
  int volumePercent;
  struct TimeInfo time;
  SDL_TimerID update_timer;
  bool shouldLoop;
  bool scrubberDragging;
};
struct PreviewWinState {
  std::string content_type;
  struct {
    unsigned char *data;
    size_t size;
    std::string path;
    std::string ext;
    std::string fileName;
    union {
      const struct Elf32_Header *elf32;
      const struct Elf64_Header *elf64;
    } elf_header;
    ElfFile *elfFile;
  } contents;
  struct PWinStateAudio audio;
  struct PWinStateTexture texture;
};
struct UIError {
  std::string message;
  std::string title;
  bool show = false;
};

inline PreviewWinState preview_state = {
    .content_type = "",
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
      .size = {0, 0},
      .anim = {},
      .frame = 0,
      .last_frame_time = 0,
      .firstFrame = true,
    }
};
inline ExtractorManager extractor_manager;

inline ArchiveBase *loaded_arc_base = nullptr;
inline unsigned char *current_buffer = nullptr;
inline Entry *selected_entry = nullptr;

inline TextEditor editor;

inline UIError ui_error = {
  .message = "",
  .title = "",
  .show = false
};

#ifdef linux
inline int inotify_fd;
inline int inotify_wd;
#endif
