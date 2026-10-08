#include "DrawPrimitives.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace esp_draw {
namespace {

constexpr ImU32 kOutlineColor = IM_COL32(0, 0, 0, 220);

float SnapHalf(float value) {
    return std::floor(value) + 0.5f;
}

ImVec2 SnapHalf(ImVec2 point) {
    return { SnapHalf(point.x), SnapHalf(point.y) };
}

ImVec2 AlignText(ImFont& font, float font_size, ImVec2 position, std::string_view text, TextAlign align) {
    const ImVec2 size = font.CalcTextSizeA(
        font_size,
        std::numeric_limits<float>::max(),
        0.0f,
        text.data(),
        text.data() + text.size()
    );

    if (align == TextAlign::Center)
        position.x -= size.x * 0.5f;
    else if (align == TextAlign::Right)
        position.x -= size.x;

    return position;
}

} // namespace

float DistanceAlpha(float distance, float fade_start, float fade_end) {
    if (!std::isfinite(distance) || !std::isfinite(fade_start) || !std::isfinite(fade_end))
        return 0.0f;

    if (fade_end <= fade_start)
        return 1.0f;

    const float progress = (distance - fade_start) / (fade_end - fade_start);
    return std::clamp(1.0f - progress, 0.0f, 1.0f);
}

ImVec2 ClampPoint(ImVec2 point, ImVec2 viewport, float margin) {
    margin = std::max(0.0f, margin);
    if (viewport.x <= margin * 2.0f || viewport.y <= margin * 2.0f)
        return { std::max(viewport.x * 0.5f, 0.0f), std::max(viewport.y * 0.5f, 0.0f) };

    return {
        std::clamp(point.x, margin, viewport.x - margin),
        std::clamp(point.y, margin, viewport.y - margin),
    };
}

ImVec2 NormalizeDirection(ImVec2 direction) {
    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length <= 0.001f || !std::isfinite(length))
        return {};

    return { direction.x / length, direction.y / length };
}

std::vector<LineSegment> BuildCornerSegments(const ScreenRect& rect, float corner_ratio) {
    if (!rect.Valid())
        return {};

    const float ratio = std::clamp(corner_ratio, 0.1f, 0.5f);
    const float corner_x = rect.Width() * ratio;
    const float corner_y = rect.Height() * ratio;
    const ImVec2 min = SnapHalf(rect.min);
    const ImVec2 max = SnapHalf(rect.max);

    return {
        { min, { SnapHalf(min.x + corner_x), min.y } },
        { min, { min.x, SnapHalf(min.y + corner_y) } },
        { { SnapHalf(max.x - corner_x), min.y }, { max.x, min.y } },
        { { max.x, min.y }, { max.x, SnapHalf(min.y + corner_y) } },
        { { min.x, SnapHalf(max.y - corner_y) }, { min.x, max.y } },
        { { min.x, max.y }, { SnapHalf(min.x + corner_x), max.y } },
        { { SnapHalf(max.x - corner_x), max.y }, max },
        { { max.x, SnapHalf(max.y - corner_y) }, max },
    };
}

void AddOutlinedText(
    ImDrawList& draw_list,
    ImFont* font,
    float font_size,
    ImVec2 position,
    ImU32 color,
    std::string_view text,
    TextAlign align
) {
    if (text.empty())
        return;

    if (!font)
        font = ImGui::GetFont();
    if (!font)
        return;

    if (font_size <= 0.0f)
        font_size = ImGui::GetFontSize();

    position = AlignText(*font, font_size, position, text, align);
    const char* begin = text.data();
    const char* end = begin + text.size();

    draw_list.AddText(font, font_size, { position.x - 1.0f, position.y }, kOutlineColor, begin, end);
    draw_list.AddText(font, font_size, { position.x + 1.0f, position.y }, kOutlineColor, begin, end);
    draw_list.AddText(font, font_size, { position.x, position.y - 1.0f }, kOutlineColor, begin, end);
    draw_list.AddText(font, font_size, { position.x, position.y + 1.0f }, kOutlineColor, begin, end);
    draw_list.AddText(font, font_size, position, color, begin, end);
}

void AddBox(ImDrawList& draw_list, const ScreenRect& rect, ImU32 color, float thickness, bool outline) {
    if (!rect.Valid())
        return;

    const ImVec2 min = SnapHalf(rect.min);
    const ImVec2 max = SnapHalf(rect.max);
    thickness = std::max(1.0f, thickness);

    if (outline)
        draw_list.AddRect(min, max, kOutlineColor, 0.0f, 0, thickness + 2.0f);
    draw_list.AddRect(min, max, color, 0.0f, 0, thickness);
}

void AddCornerBox(
    ImDrawList& draw_list,
    const ScreenRect& rect,
    ImU32 color,
    float thickness,
    float corner_ratio,
    bool outline
) {
    thickness = std::max(1.0f, thickness);
    for (const auto& segment : BuildCornerSegments(rect, corner_ratio)) {
        if (outline)
            draw_list.AddLine(segment.from, segment.to, kOutlineColor, thickness + 2.0f);
        draw_list.AddLine(segment.from, segment.to, color, thickness);
    }
}

void AddBar(
    ImDrawList& draw_list,
    const ScreenRect& rect,
    float fraction,
    ImU32 fill,
    BarSide side,
    float thickness,
    bool numeric_label
) {
    if (!rect.Valid())
        return;

    fraction = std::clamp(fraction, 0.0f, 1.0f);
    thickness = std::max(2.0f, thickness);

    ImVec2 background_min{};
    ImVec2 background_max{};
    ImVec2 fill_min{};
    ImVec2 fill_max{};

    if (side == BarSide::Left || side == BarSide::Right) {
        const float x = side == BarSide::Left ? rect.min.x - thickness - 3.0f : rect.max.x + 3.0f;
        background_min = { x, rect.min.y };
        background_max = { x + thickness, rect.max.y };
        fill_min = { x, rect.max.y - rect.Height() * fraction };
        fill_max = background_max;
    } else {
        const float y = side == BarSide::Top ? rect.min.y - thickness - 3.0f : rect.max.y + 3.0f;
        background_min = { rect.min.x, y };
        background_max = { rect.max.x, y + thickness };
        fill_min = background_min;
        fill_max = { rect.min.x + rect.Width() * fraction, y + thickness };
    }

    draw_list.AddRectFilled(background_min, background_max, kOutlineColor);
    draw_list.AddRectFilled(fill_min, fill_max, fill);

    if (numeric_label) {
        char value[8]{};
        std::snprintf(value, sizeof(value), "%d", static_cast<int>(std::round(fraction * 100.0f)));
        AddOutlinedText(
            draw_list,
            nullptr,
            10.0f,
            { background_min.x + (background_max.x - background_min.x) * 0.5f, fill_min.y - 5.0f },
            IM_COL32_WHITE,
            value,
            TextAlign::Center
        );
    }
}

bool AddOffscreenIndicator(
    ImDrawList& draw_list,
    ImVec2 viewport,
    ImVec2 projected,
    float distance,
    ImU32 color,
    float margin
) {
    if (viewport.x <= 0.0f || viewport.y <= 0.0f)
        return false;

    if (projected.x >= 0.0f && projected.x <= viewport.x && projected.y >= 0.0f && projected.y <= viewport.y)
        return false;

    const ImVec2 center{ viewport.x * 0.5f, viewport.y * 0.5f };
    const ImVec2 direction = NormalizeDirection({ projected.x - center.x, projected.y - center.y });
    if (direction.x == 0.0f && direction.y == 0.0f)
        return false;

    const float safe_margin = std::max(14.0f, margin);
    const float half_width = std::max(viewport.x * 0.5f - safe_margin, 1.0f);
    const float half_height = std::max(viewport.y * 0.5f - safe_margin, 1.0f);
    const float scale_x = direction.x == 0.0f ? std::numeric_limits<float>::max() : half_width / std::abs(direction.x);
    const float scale_y = direction.y == 0.0f ? std::numeric_limits<float>::max() : half_height / std::abs(direction.y);
    const float scale = std::min(scale_x, scale_y);
    const ImVec2 tip = ClampPoint({ center.x + direction.x * scale, center.y + direction.y * scale }, viewport, safe_margin);
    const ImVec2 tangent{ -direction.y, direction.x };
    const ImVec2 base{ tip.x - direction.x * 11.0f, tip.y - direction.y * 11.0f };

    draw_list.AddTriangleFilled(
        tip,
        { base.x + tangent.x * 5.0f, base.y + tangent.y * 5.0f },
        { base.x - tangent.x * 5.0f, base.y - tangent.y * 5.0f },
        color
    );

    char label[24]{};
    std::snprintf(label, sizeof(label), "%.0fm", std::max(0.0f, distance));
    AddOutlinedText(
        draw_list,
        nullptr,
        11.0f,
        { base.x - direction.x * 8.0f, base.y - direction.y * 8.0f },
        color,
        label,
        TextAlign::Center
    );
    return true;
}

} // namespace esp_draw
