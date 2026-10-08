#include <cassert>
#include <iostream>

#include "core/version/AppVersion.hpp"
#include "gui/frontend/brand/PlutoBrand.hpp"

int main() {
    static_assert(app_version::current == app_version::SemanticVersion{ 2, 5, 0 });
    static_assert(app_version::current_text == "2.5.0");

    const auto normal = PlutoBrand::CalculateLayout(
        { 0.0f, 0.0f, 1150.0f, 60.0f },
        { 38.0f, 38.0f },
        { 72.0f, 22.0f },
        { 42.0f, 14.0f });
    assert(normal.logo.max_x <= 1130.0f);
    assert(normal.version.min_x > normal.wordmark.max_x);
    assert(normal.logo.min_x > 4.0f * 70.0f + 20.0f);
    assert(normal.show_version);

    const auto narrow = PlutoBrand::CalculateLayout(
        { 0.0f, 0.0f, 420.0f, 60.0f },
        { 38.0f, 38.0f },
        { 72.0f, 22.0f },
        { 42.0f, 14.0f });
    assert(!narrow.show_version);
    assert(!narrow.show_wordmark);
    assert(narrow.logo.min_x > 300.0f);
    assert(narrow.wordmark.max_x <= 400.0f);

    std::cout << "Pluto brand tests passed\n";
    return 0;
}
