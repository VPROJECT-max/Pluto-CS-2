#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

#include "gui/frontend/esp/DrawPrimitives.hpp"
#include "gui/theme/Theme.hpp"

int main() {
    using namespace esp_draw;

    assert(DistanceAlpha(10.0f, 100.0f, 500.0f) == 1.0f);
    assert(DistanceAlpha(500.0f, 100.0f, 500.0f) == 0.0f);
    assert(DistanceAlpha(100.0f, 100.0f, 500.0f) == 1.0f);
    assert(DistanceAlpha(900.0f, 100.0f, 500.0f) == 0.0f);
    assert(std::abs(DistanceAlpha(300.0f, 100.0f, 500.0f) - 0.5f) < 0.001f);
    assert(DistanceAlpha(300.0f, 500.0f, 100.0f) == 1.0f);
    assert(DistanceAlpha(std::numeric_limits<float>::quiet_NaN(), 100.0f, 500.0f) == 0.0f);

    const auto clamped = ClampPoint({ -50.0f, 1200.0f }, { 1920.0f, 1080.0f }, 12.0f);
    assert(clamped.x == 12.0f);
    assert(clamped.y == 1068.0f);
    const auto zero_viewport = ClampPoint({ 80.0f, 20.0f }, { 0.0f, 0.0f }, 12.0f);
    assert(zero_viewport.x == 0.0f && zero_viewport.y == 0.0f);
    const auto tiny_viewport = ClampPoint({ 80.0f, 20.0f }, { 10.0f, 6.0f }, 12.0f);
    assert(tiny_viewport.x == 5.0f && tiny_viewport.y == 3.0f);

    const ScreenRect box{ { 100.5f, 200.5f }, { 200.5f, 400.5f } };
    const auto segments = BuildCornerSegments(box, 0.25f);
    assert(segments.size() == 8);
    assert(segments[0].from.x == 100.5f);
    assert(segments[0].to.x == 125.5f);
    const auto short_segments = BuildCornerSegments(box, -1.0f);
    assert(short_segments[0].to.x == 110.5f);
    const auto long_segments = BuildCornerSegments(box, 2.0f);
    assert(long_segments[0].to.x == 150.5f);

    const ScreenRect invalid{ { 200.0f, 400.0f }, { 100.0f, 200.0f } };
    assert(BuildCornerSegments(invalid, 0.25f).empty());

    const auto direction = NormalizeDirection({ 3.0f, 4.0f });
    assert(std::abs(direction.x - 0.6f) < 0.001f);
    assert(std::abs(direction.y - 0.8f) < 0.001f);
    const auto empty_direction = NormalizeDirection({ 0.0f, 0.0f });
    assert(empty_direction.x == 0.0f && empty_direction.y == 0.0f);

    assert(ui::theme::ClampDpiScale(0.5f) == 1.0f);
    assert(ui::theme::ClampDpiScale(1.25f) == 1.25f);
    assert(ui::theme::ClampDpiScale(3.0f) == 2.0f);

    ImGui::CreateContext();
    ui::theme::Apply(1.25f, true);
    const ImGuiStyle& style = ImGui::GetStyle();
    assert(std::abs(style.WindowPadding.x - 15.0f) < 0.001f);
    assert(std::abs(style.FrameRounding - 5.0f) < 0.001f);
    assert(style.Colors[ImGuiCol_WindowBg].x < 0.03f);
    assert(style.Colors[ImGuiCol_Text].x > 0.9f);
    ImGui::DestroyContext();

    std::cout << "geometry tests passed\n";
    return 0;
}
