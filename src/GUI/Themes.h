#pragma once

#include <vector>

enum Theme {
    BessDark,
    CatpuccinMocha
};



class ThemeManager {
    public:
        int currentTheme;
        std::vector<const char*> themes;

        void AdjustmentStyles();

        void LoadThemes() {
            themes.push_back("BessDark");
            themes.push_back("Catpuccin Mocha");
            // TODO: take in parameter to this function to load the actual current theme.
            currentTheme = Theme::BessDark;
        }

        inline void SetTheme(Theme theme) {
            if (theme == Theme::BessDark) {
                BessDark();
            }
            if (theme == Theme::CatpuccinMocha) {
                CatpuccinMocha();
            }
            currentTheme = theme;
        };
        Theme GetCurrentTheme() {
            return (Theme)currentTheme;
        }
    private:
        void BessDark();
        void CatpuccinMocha();
};
