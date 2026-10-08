#include "external_esp_menu.h"

#include "menu_framework.h"
#include "imgui_text_renderer.h"

#include "config/Config.hpp"
#include "config/Current.hpp"
#include "core/version/AppVersion.hpp"
#include "gui/frontend/brand/PlutoBrand.hpp"

#include <cstdio>
#include <ctime>
#include <cmath>

class EspPreview {
public:
    static void Render(ImVec2 size);
};

namespace ExternalEspBridge {
void ApplyStreamproof(bool enabled);
void ApplyVsync(bool enabled);
}

namespace {

void BeginColumnGroup(const char* name, const ImVec2& position, const float width_percent, const float height_percent = 1.0f) {
    Starline::GUI::State::Get().cursorPos = position;
    Starline::GUI::BeginGroup(name, width_percent, height_percent);
}

void BeginFixedGroup(const char* name, const ImVec2& position, const float width, const float height) {
    Starline::GUI::State::Get().cursorPos = position;
    Starline::GUI::BeginGroupFixed(name, width, height);
}

void RenderVisualFooter() {
    using namespace Starline;
    using namespace Starline::GUI;
    State& state = State::Get();
    Style& style = Style::Get();
    const float footer_y = state.windowPos.y + state.windowSize.y - style.footerHeight;
    state.drawList->AddRectFilled(
        { state.windowPos.x, footer_y },
        { state.windowPos.x + state.windowSize.x, state.windowPos.y + state.windowSize.y },
        Colors::Background());
    state.drawList->AddLine(
        { state.windowPos.x, footer_y },
        { state.windowPos.x + state.windowSize.x, footer_y },
        Colors::Border(), 1.0f);
    SubTab("players", 0, &g_Config.activeSubTab);
    SubTab("style", 1, &g_Config.activeSubTab);
}

void RenderVisualPlayers(
    const ImVec2& left,
    const ImVec2& middle,
    const float column_width,
    const float content_height) {
    using namespace Starline::GUI;
    BeginFixedGroup("player esp", left, column_width, content_height);
    Toggle("master enable", &cfg::enabled);
    Toggle("show team", &cfg::esp::team);
    Toggle("spotted only", &cfg::esp::spotted);
    Toggle("visible colors", &cfg::esp::visible_check);
    Toggle("box", &cfg::esp::box);
    Toggle("skeleton", &cfg::esp::skeleton);
    Toggle("head marker", &cfg::esp::head_tracker);
    Toggle("eye direction", &cfg::esp::eye_ray);
    Toggle("tracers", &cfg::esp::tracers);
    Toggle("silhouette overlay", &cfg::esp::chams);
    Toggle("outline", &cfg::esp::outline);
    Toggle("offscreen indicators", &cfg::esp::offscreen_indicators);
    EndGroup();

    BeginFixedGroup("bars and labels", middle, column_width, content_height);
    Toggle("health bar", &cfg::esp::health);
    Toggle("health number", &cfg::esp::health_number);
    Toggle("armor bar", &cfg::esp::armor);
    Toggle("name", &cfg::esp::flags::name);
    Toggle("weapon", &cfg::esp::flags::weapon);
    Toggle("ammo", &cfg::esp::flags::ammo);
    Toggle("distance", &cfg::esp::flags::distance);
    Toggle("ping", &cfg::esp::flags::ping);
    Toggle("money", &cfg::esp::flags::money);
    Toggle("flashed", &cfg::esp::flags::flashed);
    Toggle("reloading", &cfg::esp::flags::reloading);
    Toggle("defusing", &cfg::esp::flags::defusing);
    Toggle("scoped", &cfg::esp::flags::scoped);
    Toggle("c4 carrier", &cfg::esp::flags::has_c4);
    EndGroup();
}

void RenderVisualStyle(
    const ImVec2& left,
    const ImVec2& middle,
    const float column_width,
    const float content_height) {
    using namespace Starline::GUI;
    BeginFixedGroup("shape and distance", left, column_width, content_height);
    static bool corner_box = cfg::esp::box_style == 1;
    if (Toggle("corner box", &corner_box))
        cfg::esp::box_style = corner_box ? 1 : 0;
    Slider("box thickness", &cfg::esp::box_thickness, 1.0f, 6.0f, "%.1f px");
    Slider("skeleton thickness", &cfg::esp::skeleton_thickness, 1.0f, 6.0f, "%.1f px");
    Slider("bar thickness", &cfg::esp::bar_thickness, 1.0f, 6.0f, "%.1f px");
    Slider("text scale", &cfg::esp::text_scale, 0.75f, 1.5f, "%.2fx");
    Slider("fade starts", &cfg::esp::fade_start, 0.0f, 500.0f, "%.0f m");
    Slider("fade ends", &cfg::esp::fade_end, 1.0f, 500.0f, "%.0f m");
    Slider("maximum distance", &cfg::esp::max_distance, 1.0f, 500.0f, "%.0f m");
    EndGroup();

    BeginFixedGroup("colors", middle, column_width, content_height);
    Toggle("enemy box", &cfg::esp::box, true, &cfg::esp::colors::box_enemy.r);
    Toggle("visible enemy", &cfg::esp::visible_check, true, &cfg::esp::colors::box_enemy_visible.r);
    Toggle("team box", &cfg::esp::team, true, &cfg::esp::colors::box_team.r);
    Toggle("enemy skeleton", &cfg::esp::skeleton, true, &cfg::esp::colors::skeleton_enemy.r);
    Toggle("team skeleton", &cfg::esp::team, true, &cfg::esp::colors::skeleton_team.r);
    Toggle("enemy tracer", &cfg::esp::tracers, true, &cfg::esp::colors::tracer_enemy.r);
    Toggle("enemy silhouette", &cfg::esp::chams, true, &cfg::esp::colors::chams_enemy.r);
    Toggle("enemy eye ray", &cfg::esp::eye_ray, true, &cfg::esp::colors::eye_ray_enemy.r);
    EndGroup();
}

void RenderVisualPreview(const ImVec2& start, const float width, const float content_height) {
    using namespace Starline;
    using namespace Starline::GUI;
    State& state = State::Get();
    Style& style = Style::Get();
    BeginFixedGroup("live esp preview", start, width, content_height);
    const float available_width = width - style.panelPadding * 2.0f;
    const float available_height = content_height - style.panelHeaderHeight - style.panelPadding * 2.0f;
    ImGui::SetCursorScreenPos({ start.x + style.panelPadding, start.y + style.panelHeaderHeight + style.panelPadding });
    EspPreview::Render({ available_width, available_height });
    EndGroup();
}

void RenderVisuals(const ImVec2& left, const float available_width, const float content_height) {
    using namespace Starline::GUI;
    const Starline::Style& style = Starline::Style::Get();
    constexpr float preview_width = 340.0f;
    const float settings_width = available_width - preview_width - style.panelPadding;
    const float column_width = (settings_width - style.panelPadding) * 0.5f;
    const ImVec2 middle{ left.x + column_width + style.panelPadding, left.y };
    const ImVec2 preview{ middle.x + column_width + style.panelPadding, left.y };

    switch (Starline::g_Config.activeSubTab) {
    case 0: RenderVisualPlayers(left, middle, column_width, content_height); break;
    default: RenderVisualStyle(left, middle, column_width, content_height); break;
    }
    RenderVisualPreview(preview, preview_width, content_height);
    RenderVisualFooter();
}

void RenderWorld(const ImVec2& left, const ImVec2& right) {
    using namespace Starline::GUI;
    BeginColumnGroup("spectators and bomb", left, 0.5f);
    Toggle("spectator list", &cfg::world::spectators::enabled);
    Toggle("detailed spectators", &cfg::world::spectators::detailed);
    Toggle("local player only", &cfg::world::spectators::self_only);
    Toggle("bomb location", &cfg::world::bomb::location);
    Toggle("bomb timer", &cfg::world::bomb::timer);
    Toggle("sniper crosshair", &cfg::world::crosshair::enabled);
    EndGroup();

    BeginColumnGroup("radar and movement", right, 0.5f);
    Toggle("radar", &cfg::world::radar::enabled);
    Toggle("disable radar rotation", &cfg::world::radar::no_rotate);
    Slider("radar range", &cfg::world::radar::range, 100.0f, 8000.0f, "%.0f u");
    Slider("radar width", &cfg::world::radar::size.x, 120.0f, 600.0f, "%.0f px");
    Slider("radar height", &cfg::world::radar::size.y, 120.0f, 600.0f, "%.0f px");
    Toggle("velocity graph", &cfg::world::velocity::enabled);
    Slider("history length", &cfg::world::velocity::sample_length, 1.0f, 15.0f, "%.1f s");
    EndGroup();
}

void RenderSystem(const ImVec2& left, const ImVec2& right) {
    using namespace Starline::GUI;
    BeginColumnGroup("application", left, 0.5f);
    if (Toggle("hide from capture", &cfg::settings::streamproof))
        ExternalEspBridge::ApplyStreamproof(cfg::settings::streamproof);
    Toggle("watermark", &cfg::settings::watermark);
    if (Toggle("vsync", &cfg::settings::vsync))
        ExternalEspBridge::ApplyVsync(cfg::settings::vsync);
    Toggle("reduce cpu use", &cfg::settings::free_cpu);
    Toggle("force third person", &cfg::settings::force_third_person);
    Toggle("defusal notification", &cfg::settings::defusal_notification);
    EndGroup();

    BeginColumnGroup("developer", right, 0.5f);
    Toggle("console", &cfg::dev::console);
    Toggle("force player flags", &cfg::dev::force_show_flags);
    static float cache_rate = static_cast<float>(cfg::dev::cache_refresh_rate);
    if (Slider("cache refresh", &cache_rate, 1.0f, 100.0f, "%.0f ms"))
        cfg::dev::cache_refresh_rate = static_cast<int>(std::round(cache_rate));
    EndGroup();
}

void RenderConfig(const ImVec2& left, const ImVec2& right) {
    using namespace Starline::GUI;
    BeginColumnGroup("configuration", left, 0.5f);
    if (Button("save now"))
        ::Config::Write();
    if (Button("reload"))
        ::Config::Read();
    EndGroup();

    BeginColumnGroup("reset", right, 0.5f);
    static bool confirm_reset_all = false;
    if (!confirm_reset_all) {
        if (Button("reset visuals"))
            ::Config::ResetEsp();
        if (Button("reset world"))
            ::Config::ResetWorld();
        if (Button("reset system"))
            ::Config::ResetSettings();
        if (Button("reset everything"))
            confirm_reset_all = true;
    } else {
        if (Button("confirm reset everything")) {
            ::Config::ResetAll();
            confirm_reset_all = false;
        }
        if (Button("cancel"))
            confirm_reset_all = false;
    }
    EndGroup();
}

} // namespace

namespace Starline::GUI {

void InitializeExternalEsp() {
    Initialize();
    g_Config.activeMainTab = 0;
    g_Config.activeSubTab = 0;
    g_Config.menuOpen = true;
    g_Config.menuOpenAnim = 1.0f;
}

void RenderPlutoWatermark(
    const float fps,
    const float cpu_usage,
    const float working_set_mib) {
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    constexpr float screen_padding = 15.0f;
    constexpr float height = 32.0f;
    constexpr float item_spacing = 25.0f;
    constexpr float label_spacing = 2.0f;
    constexpr float inner_padding = 15.0f;
    const ImU32 background = IM_COL32(20, 20, 20, 220);
    const ImU32 border = Colors::Border();
    const ImU32 white = IM_COL32(255, 255, 255, 255);
    const ImU32 gray = IM_COL32(160, 160, 160, 255);
    const ImU32 yellow = IM_COL32(255, 200, 50, 255);
    const ImU32 outline = IM_COL32(0, 0, 0, 200);

    const std::time_t now = std::time(nullptr);
    std::tm local_time{};
    localtime_s(&local_time, &now);
    char time_text[16]{};
    std::strftime(time_text, sizeof(time_text), "%H:%M:%S", &local_time);

    char fps_text[16]{};
    char cpu_text[16]{};
    char memory_text[20]{};
    char version_text[16]{};
    std::snprintf(fps_text, sizeof(fps_text), "%.0f", fps);
    std::snprintf(cpu_text, sizeof(cpu_text), "%.0f%%", cpu_usage);
    std::snprintf(memory_text, sizeof(memory_text), "%.0fMB", working_set_mib);
    std::snprintf(version_text, sizeof(version_text), "v%s", app_version::current_text.data());

    TextFont bold_font = g_TextFont;
    bold_font.weight = FW_BOLD;
    constexpr const char* brand_name = "Pluto";
    const char* version_name = version_text;
    const ImVec2 brand_size = g_TextRenderer.MeasureText(brand_name, bold_font);
    const ImVec2 version_size = g_TextRenderer.MeasureText(version_name, g_TextFont);
    const ImVec2 fps_size = g_TextRenderer.MeasureText(fps_text, bold_font);
    const ImVec2 fps_label_size = g_TextRenderer.MeasureText("FPS", g_TextFont);
    const ImVec2 cpu_size = g_TextRenderer.MeasureText(cpu_text, bold_font);
    const ImVec2 cpu_label_size = g_TextRenderer.MeasureText("CPU", g_TextFont);
    const ImVec2 memory_size = g_TextRenderer.MeasureText(memory_text, bold_font);
    const ImVec2 memory_label_size = g_TextRenderer.MeasureText("RAM", g_TextFont);
    const ImVec2 time_size = g_TextRenderer.MeasureText(time_text, bold_font);

    const float total_width = inner_padding * 2.0f
        + brand_size.x + 16.0f + version_size.x + item_spacing
        + fps_size.x + label_spacing + fps_label_size.x + item_spacing
        + cpu_size.x + label_spacing + cpu_label_size.x + item_spacing
        + memory_size.x + label_spacing + memory_label_size.x + item_spacing
        + time_size.x;
    const ImVec2 watermark_min{ io.DisplaySize.x - total_width - screen_padding, screen_padding };
    const ImVec2 watermark_max{ io.DisplaySize.x - screen_padding, screen_padding + height };
    draw_list->AddRectFilled(watermark_min, watermark_max, background);
    draw_list->AddRect(watermark_min, watermark_max, border);

    const float gradient_width = total_width * 0.3f;
    for (int index = 0; index < static_cast<int>(total_width); ++index) {
        const ImU32 line_color = index < static_cast<int>(gradient_width)
            ? Colors::LerpColor(Colors::Accent(), border, static_cast<float>(index) / gradient_width)
            : border;
        draw_list->AddLine(
            { watermark_min.x + index, watermark_min.y },
            { watermark_min.x + index + 1.0f, watermark_min.y },
            line_color);
    }

    float x = watermark_min.x + inner_padding;
    const float text_y = watermark_min.y + (height - g_TextFont.size) * 0.5f;
    const auto render_label = [&](const char* label, const float label_x) {
        if (g_PixelFont) {
            const float label_y = watermark_min.y + (height - g_PixelFont->LegacySize) * 0.5f;
            draw_list->AddText(g_PixelFont, g_PixelFont->LegacySize, { label_x - 1.0f, label_y }, outline, label);
            draw_list->AddText(g_PixelFont, g_PixelFont->LegacySize, { label_x + 1.0f, label_y }, outline, label);
            draw_list->AddText(g_PixelFont, g_PixelFont->LegacySize, { label_x, label_y - 1.0f }, outline, label);
            draw_list->AddText(g_PixelFont, g_PixelFont->LegacySize, { label_x, label_y + 1.0f }, outline, label);
            draw_list->AddText(g_PixelFont, g_PixelFont->LegacySize, { label_x, label_y }, gray, label);
        } else if (g_FontRegular) {
            const float label_y = watermark_min.y + (height - g_FontRegular->LegacySize) * 0.5f;
            draw_list->AddText(g_FontRegular, g_FontRegular->LegacySize, { label_x, label_y }, gray, label);
        }
    };

    g_TextRenderer.RenderText(draw_list, { x, text_y }, brand_name, Colors::Accent(), bold_font);
    x += brand_size.x + 4.0f;
    g_TextRenderer.RenderText(draw_list, { x, text_y }, "-", gray, g_TextFont);
    x += 12.0f;
    g_TextRenderer.RenderText(draw_list, { x, text_y }, version_name, gray, g_TextFont);
    x += version_size.x + item_spacing;

    g_TextRenderer.RenderText(draw_list, { x, text_y }, fps_text, white, g_TextFont);
    x += fps_size.x + label_spacing;
    render_label("FPS", x);
    x += fps_label_size.x + item_spacing;

    g_TextRenderer.RenderText(draw_list, { x, text_y }, cpu_text, yellow, g_TextFont);
    x += cpu_size.x + label_spacing;
    render_label("CPU", x);
    x += cpu_label_size.x + item_spacing;

    g_TextRenderer.RenderText(draw_list, { x, text_y }, memory_text, yellow, g_TextFont);
    x += memory_size.x + label_spacing;
    render_label("RAM", x);
    x += memory_label_size.x + item_spacing;

    g_TextRenderer.RenderText(draw_list, { x, text_y }, time_text, white, g_TextFont);
}

void RenderExternalEsp() {
    ImGuiIO& io = ImGui::GetIO();
    g_Config.dropdownConsumedClick = false;
    g_Config.blockWindowDrag = false;
    g_Config.anyDropdownOpen = false;
    if (!ImGui::IsMouseDown(0))
        g_Config.interactiveMouseDown = false;
    ProcessKeybinds();

    Style& style = Style::Get();
    const ImVec2 menu_size{ 1150.0f, 650.0f };
    ImGui::SetNextWindowPos(
        { (io.DisplaySize.x - menu_size.x) * 0.5f, (io.DisplaySize.y - menu_size.y) * 0.5f },
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(menu_size);
    Colors::GlobalAlpha() = 1.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::ToVec4(Colors::BackgroundDark()));
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::ToVec4(Colors::Border()));
    ImGui::Begin("##PlutoMenu", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoMove);

    BeginFrame(ImGui::GetWindowPos(), ImGui::GetWindowSize());
    State& state = State::Get();
    const ImVec2 background_max{
        state.windowPos.x + state.windowSize.x,
        state.windowPos.y + state.windowSize.y
    };
    state.drawList->AddRectFilled(state.windowPos, background_max, Colors::BackgroundDark(), 8.0f);
    RenderHeader("Pluto");
    Tab("Visuals", "", 0, &g_Config.activeMainTab);
    Tab("World", "", 1, &g_Config.activeMainTab);
    Tab("System", "", 2, &g_Config.activeMainTab);
    Tab("Config", "", 3, &g_Config.activeMainTab);
    PlutoBrand::RenderHeaderLockup(
        state.drawList,
        {
            state.windowPos.x,
            state.windowPos.y,
            state.windowPos.x + state.windowSize.x,
            state.windowPos.y + style.headerHeight
        });

    const bool has_footer = g_Config.activeMainTab == 0;
    const float content_start_y = state.windowPos.y + style.headerHeight + style.panelPadding;
    const float footer_space = has_footer ? style.footerHeight : 0.0f;
    const float content_height = state.windowSize.y - style.headerHeight - footer_space - style.panelPadding * 2.0f;
    const float available_width = state.windowSize.x - style.panelPadding * 3.0f;
    const float column_width = available_width * 0.5f;
    const ImVec2 left{ state.windowPos.x + style.panelPadding, content_start_y };
    const ImVec2 right{ left.x + column_width + style.panelPadding, content_start_y };

    switch (g_Config.activeMainTab) {
    case 0: RenderVisuals(left, available_width, content_height); break;
    case 1: RenderWorld(left, right); break;
    case 2: RenderSystem(left, right); break;
    default: RenderConfig(left, right); break;
    }

    EndFrame();
    static bool dragging = false;
    static ImVec2 drag_offset{};
    const ImVec2 window_max{ state.windowPos.x + state.windowSize.x, state.windowPos.y + state.windowSize.y };
    const bool can_start_drag = ImGui::IsMouseHoveringRect(state.windowPos, window_max)
        && !g_Config.blockWindowDrag && !g_Config.IsPopupBlocking();
    if (can_start_drag && ImGui::IsMouseClicked(0)) {
        dragging = true;
        drag_offset = { io.MousePos.x - state.windowPos.x, io.MousePos.y - state.windowPos.y };
    }
    if (dragging) {
        if (ImGui::IsMouseDown(0))
            ImGui::SetWindowPos({ io.MousePos.x - drag_offset.x, io.MousePos.y - drag_offset.y });
        else
            dragging = false;
    }
    state.drawList->AddRect(state.windowPos, window_max, Colors::Border(), 8.0f);

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    ColorPickerPopup(g_Config.activeColorEdit);
    KeybindPopup();
    Colors::GlobalAlpha() = 1.0f;
}

} // namespace Starline::GUI
