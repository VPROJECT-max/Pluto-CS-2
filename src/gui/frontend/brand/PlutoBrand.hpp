#pragma once

struct ID3D11Device;
struct ImDrawList;
struct ImFontAtlas;

namespace PlutoBrand {

struct Size {
    float x{};
    float y{};
};

struct Rect {
    float min_x{};
    float min_y{};
    float max_x{};
    float max_y{};
};

struct BrandLayout {
    Rect logo;
    Rect wordmark;
    Rect version;
    bool show_wordmark{ true };
    bool show_version{ true };
};

[[nodiscard]] constexpr BrandLayout CalculateLayout(
    const Rect header,
    const Size logo_size,
    const Size wordmark_size,
    const Size version_size) noexcept {
    constexpr float right_padding = 20.0f;
    constexpr float logo_gap = 12.0f;
    constexpr float version_gap = 8.0f;
    constexpr float navigation_end = 4.0f * 70.0f + 20.0f;
    constexpr float navigation_clearance = 16.0f;

    const float center_y = (header.min_y + header.max_y) * 0.5f;
    BrandLayout layout;
    layout.logo.max_x = header.max_x - right_padding;
    layout.logo.min_x = layout.logo.max_x - logo_size.x;
    layout.logo.min_y = center_y - logo_size.y * 0.5f;
    layout.logo.max_y = layout.logo.min_y + logo_size.y;

    const float full_version_max = layout.logo.min_x - logo_gap;
    const float full_version_min = full_version_max - version_size.x;
    const float full_wordmark_max = full_version_min - version_gap;
    const float full_wordmark_min = full_wordmark_max - wordmark_size.x;
    const float safe_left = header.min_x + navigation_end + navigation_clearance;
    layout.show_version = full_wordmark_min >= safe_left;

    layout.version.max_x = full_version_max;
    layout.version.min_x = full_version_min;
    layout.version.min_y = center_y - version_size.y * 0.5f;
    layout.version.max_y = layout.version.min_y + version_size.y;

    layout.wordmark.max_x = layout.show_version
        ? full_wordmark_max
        : layout.logo.min_x - logo_gap;
    layout.wordmark.min_x = layout.wordmark.max_x - wordmark_size.x;
    layout.wordmark.min_y = center_y - wordmark_size.y * 0.5f;
    layout.wordmark.max_y = layout.wordmark.min_y + wordmark_size.y;
    layout.show_wordmark = layout.wordmark.min_x >= safe_left;
    return layout;
}

[[nodiscard]] bool Initialize(ID3D11Device* device, ImFontAtlas& fonts);
void Shutdown();
void RenderHeaderLockup(ImDrawList* draw_list, const Rect& header);

} // namespace PlutoBrand
