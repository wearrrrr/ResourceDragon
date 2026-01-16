#include <Markdown.h>
#include <Net.h>
#include "../../vendored/imgui_md/imgui_md.h"
#include "SDL3/SDL_misc.h"
#include "SDL3/SDL_opengl.h"
#include "fmt/format.h"
#include "lunasvg.h"
#include <SDK/util/Logger.hpp>

#include <map>
#include <vector>
#include <string>
#include <stdexcept>

// Global font references
ImFont* md_font_regular = nullptr;
ImFont* md_font_bold = nullptr;
ImFont* md_font_bold_medium = nullptr;
ImFont* md_font_bold_large = nullptr;

// Cache images by href (URL)
static std::map<std::string, std::string> image_cache;

// Helper: render SVG to RGBA vector
std::vector<unsigned char> render_svg_to_rgba(const std::string& svg_data, int& out_width, int& out_height) {
    auto document = lunasvg::Document::loadFromData(svg_data);
    if (!document) {
        Logger::error("Failed to load SVG");
        return {};
    }

    out_width = static_cast<int>(document->width());
    out_height = static_cast<int>(document->height());

    auto bitmap = document->renderToBitmap(out_width, out_height);
    std::vector<unsigned char> pixels(bitmap.data(), bitmap.data() + out_width * out_height * 4);
    return pixels;
}

// Placeholder for your GPU upload
// This should upload RGBA pixels to your renderer and return ImTextureID
ImTextureID create_texture_from_rgba(const unsigned char* rgba, int width, int height) {
    // TODO: Replace with your renderer's texture creation logic
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return (ImTextureID)(intptr_t)tex;
    return 0;
}

struct rd_markdown : public imgui_md {
    ImFont* get_font() const override {
        if (m_is_table_header)
            return md_font_bold;

        switch (m_hlevel) {
            case 0: return m_is_strong ? md_font_bold : md_font_regular;
            case 1: return md_font_bold_large;
            case 2: return md_font_bold_medium;
            default: return md_font_bold;
        }
    }

    void open_url() const override {
        SDL_OpenURL(m_href.c_str());
    }

    bool get_image(image_info& nfo) const override {
        if (m_href.empty())
            return false;

        // Check if cached
        auto it = image_cache.find(m_href);
        if (it == image_cache.end()) {
            NetManager req;
            auto result = req.Get(m_href.c_str());

            if (result.ok()) {
                image_cache[m_href] = result.body;
                it = image_cache.find(m_href);
            } else {
                Logger::error("Failed to fetch image from {}\nHTTP Status: {}", m_href, result.status);
            }
        }

        // TODO: Render things other than SVGs.
        int w, h;
        auto rgba = render_svg_to_rgba(it->second, w, h);
        if (rgba.empty())
            return false;
        ImTextureID tex = create_texture_from_rgba(rgba.data(), w, h);
        if (!tex) {
            Logger::error("Failed to create texture for {}", m_href);
            return false;
        }

        nfo.texture_id = tex;
        nfo.size = ImVec2((float)w, (float)h);
        nfo.uv0 = ImVec2(0, 0);
        nfo.uv1 = ImVec2(1, 1);
        nfo.col_tint = ImVec4(1, 1, 1, 1);
        nfo.col_border = ImVec4(0, 0, 0, 0);
        return true;
    }

    void html_div(const std::string& dclass, bool e) override {
        if (dclass == "red") {
            if (e) {
                m_table_border = false;
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            } else {
                ImGui::PopStyleColor();
                m_table_border = true;
            }
        }
    }
};

void markdown(const char* str, const char* str_end) {
    static rd_markdown s_printer;
    s_printer.print(str, str_end);
}
