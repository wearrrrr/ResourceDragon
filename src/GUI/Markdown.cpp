#include <Markdown.h>

#include "util/Logger.h"

//Fonts and images (ImTextureID) must be loaded in other place
//see https://github.com/ocornut/imgui/blob/master/docs/FONTS.md
ImFont* g_font_regular = font_registry.find("UIFont")->second;
ImFont* g_font_bold = font_registry.find("UIFontBold")->second;
ImFont* g_font_bold_large = font_registry.find("UIFontBold")->second;

struct rd_markdown : public imgui_md
{
	ImFont* get_font() const override
	{
		if (m_is_table_header) {
			return g_font_bold;
		}

		switch (m_hlevel)
		{
		case 0:
			return m_is_strong ? g_font_bold : g_font_regular;
		case 1:
			return g_font_bold_large;
		default:
			return g_font_bold;
		}
	};

	void open_url() const override
	{
		//platform dependent code
		Logger::log("Would open URL: {}", m_href);
		// SDL_OpenURL(m_href.c_str());
	}

	bool get_image(image_info& nfo) const override
	{
		//use m_href to identify images
		// nfo.texture_id = g_texture1;
		// nfo.size = {40,20};
		// nfo.uv0 = { 0,0 };
		// nfo.uv1 = {1,1};
		// nfo.col_tint = { 1,1,1,1 };
		// nfo.col_border = { 0,0,0,0 };
		// return true;
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
