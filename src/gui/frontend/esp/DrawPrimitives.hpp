#pragma once

#include <array>
#include <string_view>
#include <vector>

#include <imgui.h>

namespace esp_draw {

struct ScreenRect {
    ImVec2 min{};
    ImVec2 max{};

    [[nodiscard]] float Width() const { return max.x - min.x; }
    [[nodiscard]] float Height() const { return max.y - min.y; }
    [[nodiscard]] bool Valid() const { return Width() > 0.0f && Height() > 0.0f; }
};

struct LineSegment {
    ImVec2 from{};
    ImVec2 to{};
};

enum class TextAlign {
    Left,
    Center,
    Right,
};

enum class BarSide {
    Left,
    Right,
    Top,
    Bottom,
};

[[nodiscard]] float DistanceAlpha(float distance, float fade_start, float fade_end);
[[nodiscard]] ImVec2 ClampPoint(ImVec2 point, ImVec2 viewport, float margin);
[[nodiscard]] ImVec2 NormalizeDirection(ImVec2 direction);
[[nodiscard]] std::vector<LineSegment> BuildCornerSegments(const ScreenRect& rect, float corner_ratio);

void AddOutlinedText(
    ImDrawList& draw_list,
    ImFont* font,
    float font_size,
    ImVec2 position,
    ImU32 color,
    std::string_view text,
    TextAlign align = TextAlign::Left
);

void AddBox(
    ImDrawList& draw_list,
    const ScreenRect& rect,
    ImU32 color,
    float thickness,
    bool outline
);

void AddCornerBox(
    ImDrawList& draw_list,
    const ScreenRect& rect,
    ImU32 color,
    float thickness,
    float corner_ratio,
    bool outline
);

void AddBar(
    ImDrawList& draw_list,
    const ScreenRect& rect,
    float fraction,
    ImU32 fill,
    BarSide side,
    float thickness,
    bool numeric_label
);

bool AddOffscreenIndicator(
    ImDrawList& draw_list,
    ImVec2 viewport,
    ImVec2 projected,
    float distance,
    ImU32 color,
    float margin
);

} // namespace esp_draw
