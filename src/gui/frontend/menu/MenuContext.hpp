#pragma once

#include <array>

enum class MenuPage {
    Visuals,
    World,
    System,
    Config,
};

struct MenuContext {
    MenuPage page{ MenuPage::Visuals };
    std::array<char, 96> search{};
    bool show_diagnostics{ false };
    float dpi_scale{ 1.0f };
};
