#pragma once

enum Theme {
    BessDark
};

class ThemeManager {
    public:
        static inline void SetTheme(Theme name) {
            if (name == Theme::BessDark) {
                BessDark();
            }
        };
    private:
        static void BessDark();
};
