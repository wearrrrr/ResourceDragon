#include <Markdown.h>
#include "../../vendored/imgui_md/imgui_md.h"
#include "SDL3/SDL_misc.h"
#include "util/Logger.h"

//Fonts and images (ImTextureID) must be loaded in other place
//see https://github.com/ocornut/imgui/blob/master/docs/FONTS.md

ImFont* md_font_regular;
ImFont* md_font_bold;
ImFont* md_font_bold_large;
ImFont* md_font_bold_medium;

struct rd_markdown : public imgui_md
{
	ImFont* get_font() const override
	{
		if (m_is_table_header) {
			return md_font_bold;
		}
		switch (m_hlevel)
		{
		case 0:
			return m_is_strong ? md_font_bold : md_font_regular;
		case 1:
			return md_font_bold_large;
		case 2:
			return md_font_bold_medium;
		default:
			return md_font_bold;
		}
	};

	void open_url() const override {
		SDL_OpenURL(m_href.c_str());
	}

	bool get_image(image_info& nfo) const override {
		//use m_href to identify images
		// nfo.texture_id = g_texture1;
		// nfo.size = {40,20};
		// nfo.uv0 = { 0,0 };
		// nfo.uv1 = {1,1};
		// nfo.col_tint = { 1,1,1,1 };
		// nfo.col_border = { 0,0,0,0 };
		// return true;
		Logger::log("Image loading doesn't work yet!");
		return false;
	}

	void html_div(const std::string& dclass, bool e) override
	{
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


//call this function to render your markdown
void markdown(const char* str, const char* str_end) {
	static rd_markdown s_printer;
	s_printer.print(str, str_end);
}
