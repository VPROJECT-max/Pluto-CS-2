#include <cassert>
#include <cmath>
#include <iostream>

#include "core/version/AppVersion.hpp"
#include "gui/frontend/overlays/OverlayPresentation.hpp"
#include "gui/frontend/overlays/SystemTelemetry.hpp"

int main() {
    using namespace overlay_presentation;

    const OverlayToggles all_enabled{
        .spectators = true,
        .radar = true,
        .bomb_location = true,
        .bomb_timer = true,
        .velocity = true,
    };
    const LiveGameState no_live_data{
        .local_alive = false,
        .has_spectators = false,
        .bomb_planted = false,
    };
    const OverlayVisibility persistent = ResolveVisibility(all_enabled, no_live_data);
    assert(persistent.spectators);
    assert(persistent.radar);
    assert(persistent.bomb);
    assert(persistent.velocity);

    const OverlayVisibility disabled = ResolveVisibility({}, no_live_data);
    assert(!disabled.spectators);
    assert(!disabled.radar);
    assert(!disabled.bomb);
    assert(!disabled.velocity);

    const BombPresentation idle = BuildBombPresentation(true, true, false, 'A', 40.0f);
    assert(idle.text == "BOMB IDLE");
    assert(!idle.show_progress);
    assert(idle.progress == 0.0f);

    const BombPresentation planted = BuildBombPresentation(true, true, true, 'B', 12.5f);
    assert(planted.text == "SITE B | 12.5s");
    assert(planted.show_progress);
    assert(std::abs(planted.progress - 0.3125f) < 0.0001f);

    const CoverCrop crop = CalculateCoverCrop({ 1000.0f, 555.0f }, { 340.0f, 500.0f });
    assert(std::abs(crop.uv_min.x - 0.3113f) < 0.001f);
    assert(std::abs(crop.uv_max.x - 0.6887f) < 0.001f);
    assert(crop.uv_min.y == 0.0f);
    assert(crop.uv_max.y == 1.0f);

    static_assert(app_version::current.major == 2);
    static_assert(app_version::current.minor == 5);
    static_assert(app_version::current.patch == 0);
    static_assert(app_version::current_text == "2.5.0");

    SystemTelemetry telemetry;
    const SystemTelemetrySnapshot telemetry_snapshot = telemetry.Sample();
    assert(telemetry_snapshot.cpu_percent >= 0.0f);
    assert(telemetry_snapshot.cpu_percent <= 100.0f);
    assert(telemetry_snapshot.working_set_mib > 0.0f);

    std::cout << "overlay presentation tests passed\n";
    return 0;
}
