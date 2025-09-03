#pragma once

#include <vector>

enum Theme {
    Dark,
    CatpuccinMocha
};



class ThemeManager {
    public:
        int currentTheme;
        std::vector<const char*> themes;

        void AdjustmentStyles();

        void LoadThemes() {
            themes.push_back("Dark");
            themes.push_back("Catpuccin Mocha");
        }

        inline void SetTheme(Theme theme) {
            if (theme == Theme::Dark) {
                Dark();
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
        void Dark();
        void CatpuccinMocha();
};
