#pragma once

#include <vector>
#include <string_view>

enum Theme {
    BessDark
};



class ThemeManager {
    public:
        int currentTheme;
        std::vector<const char*> themes;

        void LoadThemes() {
            themes.push_back("BessDark");
            currentTheme = Theme::BessDark;
        }

        inline void SetTheme(Theme theme) {
            if (theme == Theme::BessDark) {
                BessDark();
            }
            currentTheme = theme;
        };
        int GetCurrentTheme() {
            return currentTheme;
        }
    private:
        static void BessDark();
};
