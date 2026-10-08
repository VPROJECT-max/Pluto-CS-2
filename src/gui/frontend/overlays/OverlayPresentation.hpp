#pragma once

#include <string>

namespace overlay_presentation {

struct OverlayToggles {
    bool spectators{};
    bool radar{};
    bool bomb_location{};
    bool bomb_timer{};
    bool velocity{};
};

struct LiveGameState {
    bool local_alive{};
    bool has_spectators{};
    bool bomb_planted{};
};

struct OverlayVisibility {
    bool spectators{};
    bool radar{};
    bool bomb{};
    bool velocity{};
};

struct BombPresentation {
    std::string text;
    bool show_progress{};
    float progress{};
};

struct Point {
    float x{};
    float y{};
};

struct CoverCrop {
    Point uv_min{};
    Point uv_max{ 1.0f, 1.0f };
};

[[nodiscard]] OverlayVisibility ResolveVisibility(
    OverlayToggles toggles,
    LiveGameState live_state) noexcept;

[[nodiscard]] BombPresentation BuildBombPresentation(
    bool location_enabled,
    bool timer_enabled,
    bool is_planted,
    char site,
    float seconds_remaining);

[[nodiscard]] CoverCrop CalculateCoverCrop(Point source_size, Point destination_size) noexcept;

} // namespace overlay_presentation
