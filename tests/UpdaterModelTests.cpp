#include <cassert>
#include <iostream>
#include <string>

#include "core/version/AppVersion.hpp"
#include "core/version/SemanticVersion.hpp"
#include "updater/ReleaseModel.hpp"

#include <nlohmann/json.hpp>

namespace {

using json = nlohmann::json;

json ValidRelease() {
    return {
        { "tag_name", "v2.5.1" },
        { "draft", false },
        { "prerelease", false },
        { "assets", json::array({ {
            { "name", "Pluto-portable.exe" },
            { "browser_download_url", "https://github.com/VPROJECT-max/Pluto-CS-2/releases/download/v2.5.1/Pluto-portable.exe" },
            { "size", 42 },
            { "digest", "sha256:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" },
        } }) },
    };
}

void AssertRejected(const json& document) {
    std::string error;
    assert(!updater::ParseLatestRelease(document, error));
    assert(!error.empty());
}

} // namespace

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

    std::string error;
    const auto release = updater::ParseLatestRelease(ValidRelease(), error);
    assert(release.has_value());
    assert(error.empty());
    assert((release->version == SemanticVersion{ 2, 5, 1 }));
    assert(release->asset.size == 42);
    assert(release->asset.sha256_hex == "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    assert(updater::ShouldInstall(app_version::current, *release));

    auto equal_release = *release;
    equal_release.version = app_version::current;
    assert(!updater::ShouldInstall(app_version::current, equal_release));
    equal_release.version = SemanticVersion{ 2, 4, 99 };
    assert(!updater::ShouldInstall(app_version::current, equal_release));

    auto invalid = ValidRelease();
    invalid["draft"] = true;
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["prerelease"] = true;
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid.erase("assets");
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"].push_back(invalid["assets"].front());
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"][0]["name"] = "pluto-portable.exe";
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"][0]["browser_download_url"] = "http://github.com/VPROJECT-max/Pluto-CS-2/releases/download/v2.5.1/Pluto-portable.exe";
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"][0].erase("digest");
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"][0]["digest"] = "sha256:not-a-hash";
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"][0]["size"] = -1;
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"][0]["size"] = 0;
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["assets"][0]["size"] = 536870913ULL;
    AssertRejected(invalid);
    invalid = ValidRelease();
    invalid["tag_name"] = 251;
    AssertRejected(invalid);

    std::cout << "updater model tests passed\n";
    return 0;
}
