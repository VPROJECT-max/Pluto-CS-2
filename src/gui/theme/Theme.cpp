#include "Theme.hpp"

#include <algorithm>
#include <cmath>

namespace ui::theme {

float ClampDpiScale(float dpi_scale) {
    if (!std::isfinite(dpi_scale))
        return 1.0f;
    return std::clamp(dpi_scale, 1.0f, 2.0f);
}

void Apply(float dpi_scale, bool reduced_motion) {
    const float scale = ClampDpiScale(dpi_scale);
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = { tokens::WindowPadding * scale, tokens::WindowPadding * scale };
    style.FramePadding = { tokens::FramePaddingX * scale, tokens::FramePaddingY * scale };
    style.ItemSpacing = { tokens::ItemSpacingX * scale, tokens::ItemSpacingY * scale };
    style.ItemInnerSpacing = { 8.0f * scale, 6.0f * scale };
    style.IndentSpacing = 18.0f * scale;
    style.ScrollbarSize = tokens::ScrollbarSize * scale;
    style.GrabMinSize = 10.0f * scale;

    style.WindowRounding = tokens::WindowRounding * scale;
    style.ChildRounding = tokens::ChildRounding * scale;
    style.FrameRounding = tokens::FrameRounding * scale;
    style.PopupRounding = tokens::FrameRounding * scale;
    style.ScrollbarRounding = tokens::FrameRounding * scale;
    style.GrabRounding = tokens::FrameRounding * scale;
    style.TabRounding = tokens::FrameRounding * scale;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    style.HoverStationaryDelay = reduced_motion ? 0.0f : 0.08f;
    style.HoverDelayShort = reduced_motion ? 0.0f : 0.08f;
    style.HoverDelayNormal = reduced_motion ? 0.0f : 0.24f;

    auto& c = style.Colors;
    c[ImGuiCol_Text] = colors::Text;
    c[ImGuiCol_TextDisabled] = colors::Disabled;
    c[ImGuiCol_WindowBg] = colors::Canvas;
    c[ImGuiCol_ChildBg] = colors::Surface;
    c[ImGuiCol_PopupBg] = colors::Raised;
    c[ImGuiCol_Border] = colors::Border;
    c[ImGuiCol_BorderShadow] = { 0.0f, 0.0f, 0.0f, 0.0f };
    c[ImGuiCol_FrameBg] = colors::Raised;
    c[ImGuiCol_FrameBgHovered] = colors::Hovered;
    c[ImGuiCol_FrameBgActive] = colors::Border;
    c[ImGuiCol_TitleBg] = colors::Canvas;
    c[ImGuiCol_TitleBgActive] = colors::Canvas;
    c[ImGuiCol_TitleBgCollapsed] = colors::Canvas;
    c[ImGuiCol_MenuBarBg] = colors::Surface;
    c[ImGuiCol_ScrollbarBg] = colors::Surface;
    c[ImGuiCol_ScrollbarGrab] = colors::Border;
    c[ImGuiCol_ScrollbarGrabHovered] = colors::Disabled;
    c[ImGuiCol_ScrollbarGrabActive] = colors::Secondary;
    c[ImGuiCol_CheckMark] = colors::Canvas;
    c[ImGuiCol_SliderGrab] = colors::Accent;
    c[ImGuiCol_SliderGrabActive] = colors::Text;
    c[ImGuiCol_Button] = colors::Raised;
    c[ImGuiCol_ButtonHovered] = colors::Hovered;
    c[ImGuiCol_ButtonActive] = colors::Border;
    c[ImGuiCol_Header] = colors::Raised;
    c[ImGuiCol_HeaderHovered] = colors::Hovered;
    c[ImGuiCol_HeaderActive] = colors::Border;
    c[ImGuiCol_Separator] = colors::QuietBorder;
    c[ImGuiCol_SeparatorHovered] = colors::Border;
    c[ImGuiCol_SeparatorActive] = colors::Secondary;
    c[ImGuiCol_ResizeGrip] = { 0.0f, 0.0f, 0.0f, 0.0f };
    c[ImGuiCol_ResizeGripHovered] = colors::Border;
    c[ImGuiCol_ResizeGripActive] = colors::Secondary;
    c[ImGuiCol_Tab] = colors::Surface;
    c[ImGuiCol_TabHovered] = colors::Hovered;
    c[ImGuiCol_TabSelected] = colors::Raised;
    c[ImGuiCol_TabDimmed] = colors::Canvas;
    c[ImGuiCol_TabDimmedSelected] = colors::Surface;
    c[ImGuiCol_PlotLines] = colors::Secondary;
    c[ImGuiCol_PlotLinesHovered] = colors::Text;
    c[ImGuiCol_PlotHistogram] = colors::Secondary;
    c[ImGuiCol_PlotHistogramHovered] = colors::Text;
    c[ImGuiCol_TableHeaderBg] = colors::Raised;
    c[ImGuiCol_TableBorderStrong] = colors::Border;
    c[ImGuiCol_TableBorderLight] = colors::QuietBorder;
    c[ImGuiCol_TableRowBg] = colors::Surface;
    c[ImGuiCol_TableRowBgAlt] = colors::Raised;
    c[ImGuiCol_TextSelectedBg] = { 0.35f, 0.35f, 0.35f, 0.45f };
    c[ImGuiCol_DragDropTarget] = colors::Accent;
    c[ImGuiCol_NavCursor] = colors::Accent;
    c[ImGuiCol_NavWindowingHighlight] = colors::Text;
    c[ImGuiCol_NavWindowingDimBg] = { 0.0f, 0.0f, 0.0f, 0.65f };
    c[ImGuiCol_ModalWindowDimBg] = { 0.0f, 0.0f, 0.0f, 0.72f };
}

} // namespace ui::theme
