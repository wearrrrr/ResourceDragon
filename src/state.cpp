#include "state.h"
#include "ExtractorManager.h"

std::map<std::string, ImFont*> font_registry;

ThemeManager theme_manager;

PreviewWinState initial_preview_state = {
    .contents {
        .type = ContentType::UNKNOWN
    },
    .audio {
        .volumePercent = 100
    },
};

std::vector<PreviewWinState> preview_windows = {initial_preview_state};

ExtractorManager *extractor_manager = new ExtractorManager();

ArchiveBase *loaded_arc_base = nullptr;
u8 *current_buffer = nullptr;
Entry *selected_entry = nullptr;

PImageView image_preview = {
    .zoom = 1.0f,
    .pan = {0.0f, 0.0f}
};

bool text_editor__unsaved_changes = false;
bool fb__loading_arc = false;
std::string fb__loading_file_name = "";

bool default_to_hex_view = false;
bool text_viewer_override = false;

#ifdef __linux__
int inotify_fd;
int inotify_wd;
#endif

bool openDelPopup;
bool quitDialog;
