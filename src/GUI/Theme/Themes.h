#pragma once

#include <string>
#include "imgui.h"

class Theme {
    public:
        static void SetTheme(std::string name) {
            if (name == "BessDark") {
                BessDark();
            }
        };
    private:
        static void BessDark();
};