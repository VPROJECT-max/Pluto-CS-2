#include "ConfigDocument.hpp"

#include <algorithm>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace config_document {
namespace {

json Defaults() {
    return {
        { "schema_version", kSchemaVersion },
        { "enabled", true },
        { "esp", {
            { "team", true }, { "box", true }, { "armor", true }, { "health", true },
            { "skeleton", true }, { "head_tracker", true }, { "health_number", false },
            { "box_style", 0 }, { "box_thickness", 1.0f }, { "outline", true },
            { "offscreen_indicators", true }, { "fade_start", 45.0f }, { "fade_end", 110.0f },
            { "max_distance", 140.0f }, { "skeleton_thickness", 1.5f }, { "text_scale", 1.0f },
            { "bar_thickness", 2.0f }, { "spotted", false }, { "tracers", false },
            { "chams", false }, { "eye_ray", false }, { "visible_check", false },
            { "flags", json::object() }, { "colors", json::object() },
        } },
        { "world", json::object() },
        { "utils", { { "debug_overlay", false } } },
    };
}

void EnsureObject(json& parent, const char* key) {
    if (!parent.contains(key) || !parent[key].is_object())
        parent[key] = json::object();
}

template <typename T>
void EnsureScalar(json& object, const char* key, const T& fallback) {
    if (!object.contains(key) || !object[key].is_primitive() || object[key].is_null()) {
        object[key] = fallback;
        return;
    }

    try {
        (void)object[key].get<T>();
    } catch (...) {
        object[key] = fallback;
    }
}

template <typename T>
void Clamp(json& object, const char* key, T low, T high) {
    const T value = object[key].get<T>();
    object[key] = std::clamp(value, low, high);
}

} // namespace

json Migrate(json document) {
    if (!document.is_object())
        document = json::object();

    int version = 1;
    if (document.contains("schema_version") && document["schema_version"].is_number_integer())
        version = document["schema_version"].get<int>();

    if (version < 2 && document.contains("esp") && document["esp"].is_object()) {
        auto& colors = document["esp"]["colors"];
        if (colors.is_object() && colors.contains("flags") && colors["flags"].is_object()) {
            auto& flags = colors["flags"];
            if (flags.contains("blinded_team") && !flags.contains("flashed_team"))
                flags["flashed_team"] = flags["blinded_team"];
            if (flags.contains("blinded_enemy") && !flags.contains("flashed_enemy"))
                flags["flashed_enemy"] = flags["blinded_enemy"];
        }
    }

    document["schema_version"] = kSchemaVersion;
    return document;
}

json Normalize(json document) {
    document = Migrate(std::move(document));
    const json defaults = Defaults();

    EnsureScalar(document, "enabled", true);
    EnsureObject(document, "esp");
    EnsureObject(document, "world");
    EnsureObject(document, "utils");
    EnsureScalar(document["utils"], "debug_overlay", false);

    auto& esp = document["esp"];
    const auto& esp_defaults = defaults["esp"];
    for (auto it = esp_defaults.begin(); it != esp_defaults.end(); ++it) {
        if (it.value().is_object()) {
            EnsureObject(esp, it.key().c_str());
        } else if (it.value().is_boolean()) {
            EnsureScalar(esp, it.key().c_str(), it.value().get<bool>());
        } else if (it.value().is_number_integer()) {
            EnsureScalar(esp, it.key().c_str(), it.value().get<int>());
        } else if (it.value().is_number()) {
            EnsureScalar(esp, it.key().c_str(), it.value().get<float>());
        }
    }

    Clamp(esp, "box_style", 0, 1);
    Clamp(esp, "box_thickness", 1.0f, 6.0f);
    Clamp(esp, "bar_thickness", 1.0f, 6.0f);
    Clamp(esp, "skeleton_thickness", 1.0f, 6.0f);
    Clamp(esp, "text_scale", 0.75f, 1.5f);
    Clamp(esp, "fade_start", 0.0f, 500.0f);
    Clamp(esp, "fade_end", 0.0f, 500.0f);
    Clamp(esp, "max_distance", 0.0f, 500.0f);
    if (esp["fade_end"].get<float>() <= esp["fade_start"].get<float>())
        esp["fade_end"] = (std::min)(500.0f, esp["fade_start"].get<float>() + 1.0f);

    document["schema_version"] = kSchemaVersion;
    return document;
}

bool Read(const std::filesystem::path& path, json& document, std::string& error) {
    error.clear();
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "Unable to open configuration: " + path.string();
        return false;
    }

    try {
        json parsed;
        stream >> parsed;
        document = Normalize(Migrate(std::move(parsed)));
        return true;
    } catch (const std::exception& exception) {
        error = std::string("Invalid configuration: ") + exception.what();
        return false;
    }
}

bool WriteAtomic(const std::filesystem::path& path, const json& document, std::string& error) {
    error.clear();
    const std::filesystem::path temporary(path.string() + ".tmp");
    std::error_code ec;

    try {
        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        {
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            if (!stream) {
                error = "Unable to create temporary configuration";
                return false;
            }
            stream << Normalize(document).dump(4) << '\n';
            stream.flush();
            if (!stream) {
                error = "Unable to flush temporary configuration";
                stream.close();
                std::filesystem::remove(temporary, ec);
                return false;
            }
        }

#ifdef _WIN32
        if (MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return true;
#else
        std::filesystem::rename(temporary, path, ec);
        if (!ec)
            return true;
#endif

        ec.clear();
        std::filesystem::copy_file(temporary, path, std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            std::filesystem::remove(temporary, ec);
            return true;
        }

        error = "Unable to replace configuration: " + ec.message();
        std::filesystem::remove(temporary, ec);
        return false;
    } catch (const std::exception& exception) {
        error = std::string("Unable to write configuration: ") + exception.what();
        std::filesystem::remove(temporary, ec);
        return false;
    }
}

} // namespace config_document
