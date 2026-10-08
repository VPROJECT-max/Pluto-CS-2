#pragma once

#include <imgui.h>

#include "gui/frontend/menu/MenuContext.hpp"

class Menu {
public:
    Menu(const Menu&) = delete;
    Menu(Menu&&) = delete;
    Menu& operator=(const Menu&) = delete;
    Menu& operator=(Menu&&) = delete;

    static bool Init();
    static void Render();
    static void RenderStartupHelp();
    static ImVec2 GetPos();
    static ImVec2 GetSize();

private:
    Menu() = default;
    static Menu& GetInstance();

    bool InitImpl();
    void RenderImpl();
    void RenderStartupHelpImpl();
    void RenderNavigation();

    bool is_setup{ false };
    ImVec2 pos{};
    ImVec2 size{};
    MenuContext context{};
};
