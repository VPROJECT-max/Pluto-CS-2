#include <cassert>
#include <iostream>

#include "core/version/AppVersion.hpp"
#include "core/version/SemanticVersion.hpp"

int main() {
    using app_version::SemanticVersion;

    const auto current = app_version::ParseTag("v2.5.0");
    assert(current.has_value());
    assert((*current == SemanticVersion{ 2, 5, 0 }));
    assert(current->ToString() == "2.5.0");

    assert(!app_version::ParseTag("2.5.0"));
    assert(!app_version::ParseTag("v2.5"));
    assert(!app_version::ParseTag("v2.5.0-beta"));
    assert(!app_version::ParseTag("v4294967296.0.0"));
    assert(!app_version::ParseTag("v02.5.0"));
    assert(!app_version::ParseTag("v2.+5.0"));
    assert(!app_version::ParseTag("v2.5.0 "));

    assert((SemanticVersion{ 2, 5, 0 } > SemanticVersion{ 2, 4, 99 }));
    assert((SemanticVersion{ 3, 0, 0 } > SemanticVersion{ 2, 99, 99 }));
    assert((SemanticVersion{ 2, 5, 0 } == SemanticVersion{ 2, 5, 0 }));

    static_assert(app_version::current == SemanticVersion{ 2, 5, 0 });
    static_assert(app_version::current_text == "2.5.0");

    std::cout << "updater model tests passed\n";
    return 0;
}
