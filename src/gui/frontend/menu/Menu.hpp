#pragma once
#if 0 // Legacy menu retained for reference; the clean shell is declared below.
#include "imgui_helper.hpp"
#include "imgui_custom.hpp"
#include <d3d11.h>

#include <string>
#include <vector>

// Tab IDs used with selectedTab
enum Tab {
    LEGITBOT = 0,
    VISUALS  = 1,
    MISC     = 2,
    CONFIG   = 3
};

class Menu {
public:
    ~Menu() = default;
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
    Menu() {};

    static Menu& GetInstance()
    {
        static Menu i{};
        return i;
    }

    bool InitImpl();
    void RenderImpl();
    void RenderStartupHelpImpl();

    // -----------------------------------------------------------------------
    // Theme helpers
    // -----------------------------------------------------------------------
    void SetupStyles();
    void loadFont();
    void setColors();

    // -----------------------------------------------------------------------
    // Panel / layout helpers
    // -----------------------------------------------------------------------
    void renderPanel();      // entire left column
    void renderLogo();
    void renderTabs();
    void renderUser();

    // -----------------------------------------------------------------------
    // Tab content renderers
    // -----------------------------------------------------------------------
    void renderLegitBotTab();
    void renderVisualsTab();
    void renderMiscTab();
    void renderConfigTab();

    void loadLogo();
    void loadEspPreview();

private:
    bool isSetup = true;

    ImVec2 pos;
    ImVec2 size;

    ImFont* bigFont = nullptr;

    // Logo texture (alien logo)
    ID3D11ShaderResourceView* logoSRV   = nullptr;
    int                       logoW     = 0;
    int                       logoH     = 0;

    // ESP Preview texture
    ID3D11ShaderResourceView* espPreviewSRV = nullptr;
    int                       espPreviewW   = 0;
    int                       espPreviewH   = 0;

    // -----------------------------------------------------------------------
    // Theme colors  (mirror of new UI's ImVec4 palette)
    // -----------------------------------------------------------------------
    ImVec4 winCol            = ImGuiHelper::rgbaToVec4(0,   0,   0,   230);
    ImVec4 bgCol             = ImGuiHelper::rgbaToVec4(15,  15,  15,  255);
    ImVec4 childCol          = ImGuiHelper::rgbaToVec4(12,  12,  12,  255);
    ImVec4 childCol1         = ImGuiHelper::rgbaToVec4(10,  10,  10,  255);
    ImVec4 notSelectedTextColor = ImGuiHelper::rgbaToVec4(140, 140, 140, 255);
    ImVec4 textCol           = ImGuiHelper::rgbaToVec4(255, 255, 255, 255);
    ImVec4 btnActiveCol      = ImGuiHelper::rgbaToVec4(57, 255,  20,  255);
    ImVec4 btnHoverCol       = ImGuiHelper::rgbaToVec4(45, 200,  15,  255);
    ImVec4 frameCol          = ImGuiHelper::rgbaToVec4(20,  20,  20,  255);
    ImVec4 hoverCol          = ImGuiHelper::rgbaToVec4(25,  25,  25,  255);
    ImVec4 itemCol           = ImGuiHelper::rgbaToVec4(57, 255,  20,  255);
    ImVec4 itemActiveCol     = ImGuiHelper::rgbaToVec4(45, 200,  15,  255);
    ImVec4 resizeGripCol     = ImGuiHelper::rgbaToVec4(57, 255,  20,  120);
    ImVec4 resizeGripHoverCol= ImGuiHelper::rgbaToVec4(57, 255,  20,  140);

    // -----------------------------------------------------------------------
    // Tab state
    // -----------------------------------------------------------------------
    int selectedTab           = VISUALS;   // default to Visuals tab
    int selectedSubTabVisuals = 0;         // 0=ESP  1=World  2=Other
    int selectedSubTabMisc    = 0;         // 0=General 1=GUI

    // -----------------------------------------------------------------------
    // Search box buffer (used in renderTabs)
    // -----------------------------------------------------------------------
    char searchBuffer[128] = "";
};
#endif

#include "MenuShell.hpp"
