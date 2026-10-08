#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace config_document {

using json = nlohmann::json;

inline constexpr int kSchemaVersion = 2;

[[nodiscard]] json Migrate(json document);
[[nodiscard]] json Normalize(json document);
bool Read(const std::filesystem::path& path, json& document, std::string& error);
bool WriteAtomic(const std::filesystem::path& path, const json& document, std::string& error);

} // namespace config_document
