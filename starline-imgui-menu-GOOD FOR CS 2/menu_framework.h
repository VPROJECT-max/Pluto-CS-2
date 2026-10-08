#pragma once
#include "../src/external/imgui/imgui.h"
#include "../src/external/imgui/imgui_internal.h"
#include "fontawesome/font_awesome.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>

namespace Starline {
    namespace Colors {
        inline float& GlobalAlpha() { static float a = 1.0f; return a; }
        inline ImU32 ApplyAlpha(ImU32 col) {
            if (GlobalAlpha() >= 0.99f) return col;
            ImVec4 v = ImGui::ColorConvertU32ToFloat4(col);
            v.w *= GlobalAlpha();
            return ImGui::ColorConvertFloat4ToU32(v);
        }
        
        inline ImU32 Background()       { return ApplyAlpha(IM_COL32(15, 15, 15, 255)); }
        inline ImU32 BackgroundDark()   { return ApplyAlpha(IM_COL32(10, 10, 10, 255)); }
        inline ImU32 Accent()           { return ApplyAlpha(IM_COL32(114, 137, 218, 255)); }
        inline ImU32 AccentHover()      { return ApplyAlpha(IM_COL32(134, 157, 238, 255)); }
        inline ImU32 AccentDim()        { return ApplyAlpha(IM_COL32(114, 137, 218, (int)(100 * GlobalAlpha()))); }
        inline ImU32 TextActive()       { return ApplyAlpha(IM_COL32(255, 255, 255, 255)); }
        inline ImU32 TextInactive()     { return ApplyAlpha(IM_COL32(136, 136, 136, 255)); }
        inline ImU32 TextDim()          { return ApplyAlpha(IM_COL32(100, 100, 100, 255)); }
        inline ImU32 PanelBg()          { return ApplyAlpha(IM_COL32(20, 20, 20, 255)); }
        inline ImU32 PanelHeader()      { return ApplyAlpha(IM_COL32(30, 30, 30, 255)); }
        inline ImU32 Border()           { return ApplyAlpha(IM_COL32(40, 40, 40, 255)); }
        inline ImU32 ToggleOff()        { return ApplyAlpha(IM_COL32(50, 50, 50, 255)); }
        inline ImU32 ToggleOn()         { return ApplyAlpha(IM_COL32(114, 137, 218, 255)); }
        inline ImU32 GearIcon()         { return ApplyAlpha(IM_COL32(180, 180, 180, 255)); }
        inline ImU32 GearIconHover()    { return ApplyAlpha(IM_COL32(255, 255, 255, 255)); }
        
        inline ImVec4 ToVec4(ImU32 col) {
            return ImGui::ColorConvertU32ToFloat4(col);
        }
        
        inline ImU32 FromVec4(const ImVec4& col) {
            return ImGui::ColorConvertFloat4ToU32(col);
        }
        
        inline ImU32 WithAlpha(ImU32 col, float alpha) {
            ImVec4 v = ToVec4(col);
            v.w = alpha * GlobalAlpha();
            return FromVec4(v);
        }
        
        inline ImU32 LerpColor(ImU32 a, ImU32 b, float t) {
            ImVec4 va = ToVec4(a);
            ImVec4 vb = ToVec4(b);
            ImVec4 result(
                va.x + (vb.x - va.x) * t,
                va.y + (vb.y - va.y) * t,
                va.z + (vb.z - va.z) * t,
                (va.w + (vb.w - va.w) * t) * GlobalAlpha()
            );
            return FromVec4(result);
        }
    }
    enum class KeybindType {
        Hold = 0,
        Toggle = 1,
        Always = 2
    };
    
    struct Keybind {
        int key = 0;
        KeybindType type = KeybindType::Hold;
        bool active = false;
        
        bool IsActive() const;
        const char* GetKeyName() const;
        static const char* GetTypeName(KeybindType t);
    };
    void ProcessKeybinds();
    struct Config {
        int activeMainTab = 1;
        int activeSubTab = 0;
        bool menuOpen = true;
        struct {
            bool enabled = false;
            bool autoFire = false;
            bool autoScope = false;
            bool autoStop = false;
            bool silentAim = false;
            bool multiPoint = false;
            bool delayShot = false;
            bool preferBody = false;
            bool preferHead = false;
            float fov = 5.0f;
            float smooth = 1.0f;
            float hitchance = 70.0f;
            float minDamage = 20.0f;
        } aimbot;
        struct ESPTarget {
            bool enabled = false;
            bool onlyVisible = false;
            bool onlyAudible = false;
            bool boundingBox = false;
            bool name = false;
            bool avatar = false;
            bool healthBar = false;
            bool ammoBar = false;
            bool weaponName = false;
            bool weaponIcon = false;
            bool flags = false;
            bool grenades = false;
            bool skeleton = false;
            bool lineOfSight = false;
            bool sounds = false;
            float boundingBoxColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float nameColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float healthBarColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            float ammoBarColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float weaponNameColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float weaponIconColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float grenadesColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float skeletonColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float lineOfSightColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float soundsColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            int flagsSelection = 0;
        };
        ESPTarget espTargets[3];
        struct {
            bool enabled = false;
            bool onlyVisible = false;
            bool onlyAudible = false;
            bool boundingBox = false;
            bool name = false;
            bool avatar = false;
            bool healthBar = false;
            bool ammoBar = false;
            bool weaponName = false;
            bool weaponIcon = false;
            bool flags = false;
            bool grenades = false;
            bool skeleton = false;
            bool lineOfSight = false;
            bool sounds = false;
            
            float boundingBoxColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float nameColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float healthBarColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            float ammoBarColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float weaponNameColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float weaponIconColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float grenadesColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float skeletonColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float lineOfSightColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float soundsColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            
            int flagsSelection = 0;
            enum FlagBits {
                FLAG_ARMOR      = 1 << 0,
                FLAG_SCOPED     = 1 << 1,
                FLAG_FLASHED    = 1 << 2,
                FLAG_DEFUSING   = 1 << 3,
                FLAG_PLANTING   = 1 << 4,
                FLAG_RELOADING  = 1 << 5,
                FLAG_MONEY      = 1 << 6,
                FLAG_DISTANCE   = 1 << 7
            };
        } esp;
        struct {
            bool visibleChams = false;
            bool invisibleChams = false;
            bool overlayChams = false;
            bool backtrackChams = false;
            bool disableOcclusion = false;
            
            float visibleColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float invisibleColor[4] = {0.855f, 0.447f, 0.447f, 1.0f};
            float overlayColor[4] = {0.447f, 0.855f, 0.537f, 1.0f};
            float backtrackColor[4] = {0.855f, 0.855f, 0.447f, 1.0f};
        } chams;
        struct {
            bool glow = false;
            bool offscreenArrows = false;
            
            float glowColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
            float offscreenArrowsColor[4] = {0.447f, 0.537f, 0.855f, 1.0f};
        } other;
        struct {
            bool skinChanger = false;
            bool knifeChanger = false;
            bool gloveChanger = false;
            bool agentChanger = false;
            bool medalChanger = false;
            bool musicKit = false;
            int selectedKnife = 0;
            int selectedGlove = 0;
        } changer;
        struct {
            bool bunnyHop = false;
            bool autoStrafe = false;
            bool fakelag = false;
            bool antiAim = false;
            bool resolver = false;
            bool lagCompensation = false;
            bool revealRanks = false;
            bool autoAccept = false;
            bool clantag = false;
            bool hitSound = false;
            bool killSound = false;
            float fakelagAmount = 1.0f;
            char configName[64] = "";
            char clantagText[32] = "";
        } misc;
        float menuOpenAnim = 1.0f;
        bool showColorPicker = false;
        float* activeColorEdit = nullptr;
        ImVec2 colorPickerAnchor = ImVec2(0, 0);
        int colorPickerOpenFrame = -1;
        bool showSettingsPopup = false;
        const char* activeSettingsLabel = nullptr;
        std::map<std::string, Keybind> keybinds;
        bool showKeybindPopup = false;
        std::string activeKeybindLabel;
        ImVec2 keybindPopupAnchor = ImVec2(0, 0);
        bool isListeningForKey = false;
        int keybindPopupOpenFrame = -1;
        bool dropdownConsumedClick = false;
        bool blockWindowDrag = false;
        bool interactiveMouseDown = false;
        float colorPickerAnim = 0.0f;
        float keybindPopupAnim = 0.0f;
        int previousTab = 0;
        int previousSubTab = 0;
        float tabTransitionAnim = 1.0f;
        float subTabTransitionAnim = 1.0f;
        bool anyDropdownOpen = false;
        bool anyDropdownOpenLastFrame = false;
        bool IsPopupBlocking() const {
            return showColorPicker || showKeybindPopup || colorPickerAnim > 0.01f || keybindPopupAnim > 0.01f;
        }
    };
    extern Config g_Config;
    extern ImFont* g_IconFont;
    extern ImFont* g_FontAwesome;
    extern ImFont* g_FontRegular;
    extern ImFont* g_FontBold;
    extern ImFont* g_PixelFont;
    extern void* g_BackgroundTexture;
    extern int g_BackgroundWidth;
    extern int g_BackgroundHeight;
    namespace Icons {
        constexpr const char* Aimbot   = "A";
        constexpr const char* Visuals  = "D";
        constexpr const char* Changer  = "B";
        constexpr const char* Misc     = "G";
        constexpr const char* Gear     = ICON_FA_KEYBOARD;
        constexpr const char* Extra1   = "H";
        constexpr const char* Extra2   = "I";
    }
    struct Style {
        float windowPadding = 0.0f;
        float itemSpacing = 4.0f;
        float panelPadding = 12.0f;
        float headerHeight = 60.0f;
        float footerHeight = 40.0f;
        float tabHeight = 50.0f;
        float toggleHeight = 28.0f;
        float toggleBoxSize = 16.0f;
        float gearIconSize = 14.0f;
        float colorPreviewSize = 16.0f;
        float panelHeaderHeight = 28.0f;
        float panelRounding = 4.0f;
        float itemRounding = 3.0f;
        float fontSize = 14.0f;
        
        static Style& Get() {
            static Style instance;
            return instance;
        }
    };
    namespace GUI {
        struct State {
            ImVec2 windowPos;
            ImVec2 windowSize;
            ImVec2 cursorPos;
            float currentGroupWidth;
            float currentGroupX;
            ImDrawList* drawList;
            
            static State& Get() {
                static State instance;
                return instance;
            }
        };
        void Initialize();
        void BeginFrame(const ImVec2& pos, const ImVec2& size);
        void EndFrame();
        std::string ToLower(const char* text);
        void RenderHeader(const char* title);
        bool Tab(const char* label, const char* icon, int tabId, int* activeTab);
        bool SubTab(const char* label, int tabId, int* activeTab);
        void BeginGroup(const char* name, float widthPercent, float heightPercent = 0.0f);
        void BeginGroupFixed(const char* name, float width, float height);
        void EndGroup();
        void BeginColumns(int count, float* widths = nullptr);
        void NextColumn();
        void EndColumns();
        bool Toggle(const char* label, bool* v, bool showGear = false, float* colorPtr = nullptr);
        bool Button(const char* label);
        bool TextInput(const char* label, char* buffer, size_t bufferSize);
        bool Slider(const char* label, float* value, float minVal, float maxVal, const char* format = "%.1f");
        bool Dropdown(const char* label, const char* preview, bool* open);
        bool MultiSelectDropdown(const char* label, int* flagsValue, bool* open);
        void ColorPickerPopup(float* colorPtr);
        void SettingsPopup(const char* label);
        void KeybindPopup();
        void RenderFooter();
        void DrawGearIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
        void DrawColorPreview(ImDrawList* dl, ImVec2 pos, float width, float height, float* colorPtr);
        void DrawTabIcon(ImDrawList* dl, ImVec2 center, float size, int iconType, ImU32 color);
        void Render();
        void RenderWatermark(float fps, float cpuUsage, float gpuUsage);
        void RenderKeybindList();
    }

}
