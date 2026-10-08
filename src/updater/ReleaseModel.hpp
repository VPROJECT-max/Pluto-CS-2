#pragma once

#include "core/version/SemanticVersion.hpp"

#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace updater {

struct ReleaseAsset final {
    std::string download_url;
    std::uint64_t size{};
    std::string sha256_hex;
};

struct ReleaseInfo final {
    app_version::SemanticVersion version;
    ReleaseAsset asset;
};

[[nodiscard]] std::optional<ReleaseInfo> ParseLatestRelease(
    const nlohmann::json& document,
    std::string& error);

[[nodiscard]] bool ShouldInstall(
    app_version::SemanticVersion current,
    const ReleaseInfo& release) noexcept;

} // namespace updater
