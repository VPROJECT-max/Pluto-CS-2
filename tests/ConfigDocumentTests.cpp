#include "config/ConfigDocument.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

std::filesystem::path TestRoot() {
    return std::filesystem::temp_directory_path() / "external_esp_config_document_tests";
}

} // namespace

int main() {
    using namespace config_document;

    json legacy = {
        { "esp", { { "bar_thickness", 99.0f } } },
        { "personal", { { "keep", true } } },
    };
    const json normalized = Normalize(Migrate(legacy));
    assert(normalized["schema_version"] == kSchemaVersion);
    assert(normalized["esp"]["bar_thickness"] == 6.0f);
    assert(normalized["esp"]["box_thickness"] == 1.0f);
    assert(normalized["personal"]["keep"] == true);

    json malformed_types = {
        { "schema_version", kSchemaVersion },
        { "esp", {
            { "box", "yes" },
            { "box_style", 50 },
            { "fade_start", -20.0f },
            { "fade_end", 9999.0f },
            { "text_scale", 0.1f },
            { "custom_nested", 42 },
        } },
    };
    const json safe = Normalize(malformed_types);
    assert(safe["esp"]["box"].is_boolean());
    assert(safe["esp"]["box_style"] == 1);
    assert(safe["esp"]["fade_start"] == 0.0f);
    assert(safe["esp"]["fade_end"] == 500.0f);
    assert(safe["esp"]["text_scale"] == 0.75f);
    assert(safe["esp"]["custom_nested"] == 42);
    assert(safe["world"].is_object());
    assert(safe["utils"].is_object());
    assert(safe["utils"]["debug_overlay"] == false);

    const auto root = TestRoot();
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto malformed_path = root / "malformed.json";
    {
        std::ofstream file(malformed_path);
        file << "{ this is not json";
    }

    std::string error;
    json parsed;
    assert(!Read(malformed_path, parsed, error));
    assert(!error.empty());

    const auto valid_path = root / "valid.json";
    assert(WriteAtomic(valid_path, normalized, error));
    assert(error.empty());
    assert(!std::filesystem::exists(valid_path.string() + ".tmp"));
    assert(Read(valid_path, parsed, error));
    assert(parsed["schema_version"] == kSchemaVersion);
    assert(parsed["personal"]["keep"] == true);

    std::filesystem::remove_all(root);
    std::cout << "config document tests passed\n";
    return 0;
}
