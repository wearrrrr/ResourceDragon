#include "Themes.h"
#include <imgui.h>

void ThemeManager::AdjustmentStyles() {
    ImGuiStyle &style = ImGui::GetStyle();

    style.FrameRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.ChildRounding = 4.0f;

    style.WindowTitleAlign = ImVec2(0.50f, 0.50f);
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 22.0f;

    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 0.0f;

    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;
}

void ThemeManager::Dark() {
    ImVec4 *colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Text]               = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);
    colors[ImGuiCol_WindowBg]           = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_Border]             = ImVec4(0.28f, 0.29f, 0.30f, 0.60f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.28f, 0.30f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]= ImVec4(0.32f, 0.34f, 0.36f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);
    colors[ImGuiCol_Separator]          = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
    colors[ImGuiCol_SeparatorActive]    = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // added
    colors[ImGuiCol_ResizeGrip]         = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.44f, 0.54f, 0.64f, 1.00f);
    colors[ImGuiCol_Tab]                = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.32f, 0.42f, 0.52f, 1.00f); // added
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // added
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f); // added
    colors[ImGuiCol_PlotLines]          = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]   = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
    colors[ImGuiCol_PlotHistogram]      = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]=ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]      = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]  = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);
    colors[ImGuiCol_TableBorderLight]   = ImVec4(0.24f, 0.25f, 0.26f, 1.00f);
    colors[ImGuiCol_TableRowBg]         = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt]      = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]     = ImVec4(0.24f, 0.34f, 0.44f, 0.35f);
    colors[ImGuiCol_DragDropTarget]     = ImVec4(0.46f, 0.56f, 0.66f, 0.90f);
    colors[ImGuiCol_NavHighlight]       = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // added
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]  = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]   = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ThemeManager::CatpuccinMocha() {
    ImVec4 *colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.89f, 0.88f, 1.00f);         // Latte
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.56f, 0.52f, 1.00f); // Surface2
    colors[ImGuiCol_WindowBg] = ImVec4(0.17f, 0.14f, 0.20f, 1.00f);     // Base
    colors[ImGuiCol_ChildBg] = ImVec4(0.18f, 0.16f, 0.22f, 1.00f);      // Mantle
    colors[ImGuiCol_PopupBg] = ImVec4(0.17f, 0.14f, 0.20f, 1.00f);      // Base
    colors[ImGuiCol_Border] = ImVec4(0.27f, 0.23f, 0.29f, 1.00f);       // Overlay0
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.21f, 0.18f, 0.25f, 1.00f);              // Crust
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.20f, 0.29f, 1.00f);       // Overlay1
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.22f, 0.31f, 1.00f);        // Overlay2
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.12f, 0.18f, 1.00f);              // Mantle
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.17f, 0.15f, 0.21f, 1.00f);        // Mantle
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.12f, 0.18f, 1.00f);     // Mantle
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.17f, 0.15f, 0.22f, 1.00f);            // Base
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.17f, 0.14f, 0.20f, 1.00f);          // Base
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.21f, 0.18f, 0.25f, 1.00f);        // Crust
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.24f, 0.20f, 0.29f, 1.00f); // Overlay1
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.26f, 0.22f, 0.31f, 1.00f);  // Overlay2
    colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.66f, 0.47f, 1.00f);            // Peach
    colors[ImGuiCol_SliderGrab] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);           // Lavender
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.89f, 0.54f, 0.79f, 1.00f);     // Pink
    colors[ImGuiCol_Button] = ImVec4(0.65f, 0.34f, 0.46f, 1.00f);               // Maroon
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.71f, 0.40f, 0.52f, 1.00f);        // Red
    colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.46f, 0.58f, 1.00f);         // Pink
    colors[ImGuiCol_Header] = ImVec4(0.65f, 0.34f, 0.46f, 1.00f);               // Maroon
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.71f, 0.40f, 0.52f, 1.00f);        // Red
    colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.46f, 0.58f, 1.00f);         // Pink
    colors[ImGuiCol_Separator] = ImVec4(0.27f, 0.23f, 0.29f, 1.00f);            // Overlay0
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.95f, 0.66f, 0.47f, 1.00f);     // Peach
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.95f, 0.66f, 0.47f, 1.00f);      // Peach
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);           // Lavender
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.89f, 0.54f, 0.79f, 1.00f);    // Pink
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.61f, 0.85f, 1.00f);     // Mauve
    colors[ImGuiCol_Tab] = ImVec4(0.21f, 0.18f, 0.25f, 1.00f);                  // Crust
    colors[ImGuiCol_TabHovered] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);           // Lavender
    colors[ImGuiCol_TabActive] = ImVec4(0.76f, 0.46f, 0.58f, 1.00f);            // Pink
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.18f, 0.16f, 0.22f, 1.00f);         // Mantle
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.21f, 0.18f, 0.25f, 1.00f);   // Crust
    colors[ImGuiCol_PlotLines] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);            // Lavender
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.89f, 0.54f, 0.79f, 1.00f);     // Pink
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);        // Lavender
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.89f, 0.54f, 0.79f, 1.00f); // Pink
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);        // Mantle
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.27f, 0.23f, 0.29f, 1.00f);    // Overlay0
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);     // Surface2
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);  // Surface0
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.82f, 0.61f, 0.85f, 0.35f); // Lavender
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.95f, 0.66f, 0.47f, 0.90f); // Peach
    colors[ImGuiCol_NavHighlight] = ImVec4(0.82f, 0.61f, 0.85f, 1.00f);   // Lavender
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
