#include "state.h"
#include "ExtractorManager.h"

PreviewWinState preview_state = {
    .audio {
        .volumePercent = 100
    }
};
ExtractorManager *extractor_manager = new ExtractorManager();

ArchiveBase *loaded_arc_base = nullptr;
u8 *current_buffer = nullptr;
Entry *selected_entry = nullptr;

PImageView image_preview = {
    .zoom = 1.0f,
    .pan = {0.0f, 0.0f}
};

bool text_editor__unsaved_changes = false;

#ifdef linux
int inotify_fd;
int inotify_wd;
#endif

bool openDelPopup;
bool quitDialog;
