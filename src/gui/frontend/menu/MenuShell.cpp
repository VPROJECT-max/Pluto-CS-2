#include "Menu.hpp"

#include "common.hpp"
#include "gui/frontend/brand/PlutoBrand.hpp"
#include "gui/frontend/preview/EspPreview.hpp"
#include "gui/renderer/Renderer.hpp"
#include "gui/renderer/window/Window.hpp"
#include "gui/theme/Theme.hpp"

#include "../../../../starline-imgui-menu-GOOD FOR CS 2/badcache.h"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/external_esp_menu.h"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/fontawesome/RawAwesome6.hpp"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/imgui_text_renderer.h"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/menu_framework.h"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/smalle.h"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/verdana.h"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/verdanabold.h"

namespace ExternalEspBridge {

void ApplyStreamproof(const bool enabled) {
    Window::SetAffinity(Window::hwnd,
        enabled ? WindowAffinity::Invisible : WindowAffinity::Disabled);
}

void ApplyVsync(const bool enabled) {
    Window::SetVSync(enabled);
}

} // namespace ExternalEspBridge

Menu& Menu::GetInstance() {
    static Menu instance{};
    return instance;
}

bool Menu::Init() { return GetInstance().InitImpl(); }
void Menu::Render() { GetInstance().RenderImpl(); }
void Menu::RenderStartupHelp() { GetInstance().RenderStartupHelpImpl(); }
ImVec2 Menu::GetPos() { return GetInstance().pos; }
ImVec2 Menu::GetSize() { return GetInstance().size; }

bool Menu::InitImpl() {
    auto& io = ImGui::GetIO();
    context.dpi_scale = ui::theme::ClampDpiScale(io.DisplayFramebufferScale.x);
    ui::theme::Apply(context.dpi_scale, true);

    ImFontConfig font_config{};
    font_config.FontDataOwnedByAtlas = false;
    Starline::g_FontRegular = io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(verdana), sizeof(verdana), 14.0f, &font_config);
    font_config.FontDataOwnedByAtlas = false;
    Starline::g_FontBold = io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(verdanabold), sizeof(verdanabold), 14.0f, &font_config);

    static const ImWchar icon_ranges[] = { 0x41, 0x49, 0 };
    font_config.FontDataOwnedByAtlas = false;
    font_config.GlyphMinAdvanceX = 28.0f;
    Starline::g_IconFont = io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(badcache), sizeof(badcache), 28.0f, &font_config, icon_ranges);

    static const ImWchar awesome_ranges[] = { 0xf000, 0xf8ff, 0 };
    font_config.FontDataOwnedByAtlas = false;
    font_config.GlyphMinAdvanceX = 14.0f;
    Starline::g_FontAwesome = io.Fonts->AddFontFromMemoryCompressedTTF(
        FontAwesome6Solid_compressed_data, FontAwesome6Solid_compressed_size,
        14.0f, &font_config, awesome_ranges);

    font_config.FontDataOwnedByAtlas = false;
    font_config.GlyphMinAdvanceX = 0.0f;
    Starline::g_PixelFont = io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(smalle), sizeof(smalle), 8.0f, &font_config);

    g_TextRenderer.Init(Window::device, Window::device_context);
    g_TextFont.fontFamily = "Verdana";
    g_TextFont.size = 13;
    g_TextFont.weight = FW_NORMAL;
    g_TextFont.antialiased = true;
    if (!EspPreview::Initialize(Window::device))
        LOGF(WARNING, "Failed to initialize embedded ESP preview image");
    if (!PlutoBrand::Initialize(Window::device, *io.Fonts))
        LOGF(WARNING, "Failed to initialize the Pluto header brand");
    Starline::GUI::InitializeExternalEsp();
    is_setup = true;
    LOGF(INFO, "Successfully initialized Pluto menu...");
    return true;
}

void Menu::RenderImpl() {
    if (!is_setup)
        return;
    Starline::GUI::RenderExternalEsp();
    const auto& state = Starline::GUI::State::Get();
    pos = state.windowPos;
    size = state.windowSize;
}

void Menu::RenderStartupHelpImpl() {
    static bool dismissed = false;
    if (dismissed)
        return;
    if (Renderer::IsOpen()) {
        dismissed = true;
        return;
    }

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const char* text = "INSERT or RIGHT SHIFT  /  open menu";
    const ImVec2 text_size = ImGui::CalcTextSize(text);
    ImGui::GetBackgroundDrawList()->AddText(
        { display.x * 0.5f - text_size.x * 0.5f, 32.0f },
        IM_COL32(136, 136, 136, 255), text);
}
