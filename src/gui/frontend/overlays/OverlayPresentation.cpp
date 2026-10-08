#include "OverlayPresentation.hpp"

#include <algorithm>
#include <format>

namespace overlay_presentation {

OverlayVisibility ResolveVisibility(
    const OverlayToggles toggles,
    const LiveGameState) noexcept {
    return {
        .spectators = toggles.spectators,
        .radar = toggles.radar,
        .bomb = toggles.bomb_location || toggles.bomb_timer,
        .velocity = toggles.velocity,
    };
}

BombPresentation BuildBombPresentation(
    const bool location_enabled,
    const bool timer_enabled,
    const bool is_planted,
    const char site,
    const float seconds_remaining) {
    if (!is_planted)
        return { .text = "BOMB IDLE", .show_progress = false, .progress = 0.0f };

    const float clamped_seconds = std::clamp(seconds_remaining, 0.0f, 40.0f);
    std::string text;
    if (location_enabled)
        text = std::format("SITE {}", site == 'B' ? 'B' : 'A');
    if (timer_enabled) {
        if (!text.empty())
            text += " | ";
        text += std::format("{:.1f}s", clamped_seconds);
    }

    return {
        .text = std::move(text),
        .show_progress = timer_enabled,
        .progress = clamped_seconds / 40.0f,
    };
}

CoverCrop CalculateCoverCrop(const Point source_size, const Point destination_size) noexcept {
    if (source_size.x <= 0.0f || source_size.y <= 0.0f
        || destination_size.x <= 0.0f || destination_size.y <= 0.0f) {
        return {};
    }

    const float source_aspect = source_size.x / source_size.y;
    const float destination_aspect = destination_size.x / destination_size.y;
    if (source_aspect > destination_aspect) {
        const float visible_fraction = destination_aspect / source_aspect;
        const float inset = (1.0f - visible_fraction) * 0.5f;
        return { .uv_min = { inset, 0.0f }, .uv_max = { 1.0f - inset, 1.0f } };
    }

    const float visible_fraction = source_aspect / destination_aspect;
    const float inset = (1.0f - visible_fraction) * 0.5f;
    return { .uv_min = { 0.0f, inset }, .uv_max = { 1.0f, 1.0f - inset } };
}

} // namespace overlay_presentation
