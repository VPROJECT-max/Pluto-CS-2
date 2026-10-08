#include "ReleaseModel.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string_view>

#include <nlohmann/json.hpp>

namespace {

constexpr std::string_view kAssetName{ "Pluto-portable.exe" };
constexpr std::string_view kDownloadPrefix{
    "https://github.com/VPROJECT-max/Pluto-CS-2/releases/download/"
};
constexpr std::uint64_t kMaximumAssetBytes = 512ULL * 1024ULL * 1024ULL;

bool ReadAssetSize(const nlohmann::json& value, std::uint64_t& size) {
    if (value.is_number_unsigned()) {
        size = value.get<std::uint64_t>();
    } else if (value.is_number_integer()) {
        const std::int64_t signed_size = value.get<std::int64_t>();
        if (signed_size <= 0)
            return false;
        size = static_cast<std::uint64_t>(signed_size);
    } else {
        return false;
    }
    return size > 0 && size <= kMaximumAssetBytes;
}

bool ParseDigest(const std::string_view value, std::string& hex) {
    constexpr std::string_view prefix{ "sha256:" };
    if (!value.starts_with(prefix) || value.size() != prefix.size() + 64)
        return false;

    hex.assign(value.substr(prefix.size()));
    for (char& character : hex) {
        const unsigned char byte = static_cast<unsigned char>(character);
        if (!std::isxdigit(byte))
            return false;
        character = static_cast<char>(std::tolower(byte));
    }
    return true;
}

} // namespace

namespace updater {

std::optional<ReleaseInfo> ParseLatestRelease(
    const nlohmann::json& document,
    std::string& error) {
    error.clear();
    if (!document.is_object()) {
        error = "Latest release response is not an object";
        return std::nullopt;
    }

    const auto tag_iterator = document.find("tag_name");
    const auto draft_iterator = document.find("draft");
    const auto prerelease_iterator = document.find("prerelease");
    const auto assets_iterator = document.find("assets");
    if (tag_iterator == document.end() || !tag_iterator->is_string()
        || draft_iterator == document.end() || !draft_iterator->is_boolean()
        || prerelease_iterator == document.end() || !prerelease_iterator->is_boolean()
        || assets_iterator == document.end() || !assets_iterator->is_array()) {
        error = "Latest release response has an invalid structure";
        return std::nullopt;
    }
    if (draft_iterator->get<bool>() || prerelease_iterator->get<bool>()) {
        error = "Latest release is not a stable published release";
        return std::nullopt;
    }

    const auto version = app_version::ParseTag(tag_iterator->get_ref<const std::string&>());
    if (!version) {
        error = "Latest release tag is not strict SemVer";
        return std::nullopt;
    }

    const nlohmann::json* matching_asset = nullptr;
    for (const auto& asset : *assets_iterator) {
        if (!asset.is_object())
            continue;
        const auto name_iterator = asset.find("name");
        if (name_iterator == asset.end() || !name_iterator->is_string()
            || name_iterator->get_ref<const std::string&>() != kAssetName) {
            continue;
        }
        if (matching_asset != nullptr) {
            error = "Latest release contains duplicate Pluto portable assets";
            return std::nullopt;
        }
        matching_asset = &asset;
    }
    if (matching_asset == nullptr) {
        error = "Latest release does not contain Pluto-portable.exe";
        return std::nullopt;
    }

    const auto url_iterator = matching_asset->find("browser_download_url");
    const auto size_iterator = matching_asset->find("size");
    const auto digest_iterator = matching_asset->find("digest");
    if (url_iterator == matching_asset->end() || !url_iterator->is_string()
        || size_iterator == matching_asset->end()
        || digest_iterator == matching_asset->end() || !digest_iterator->is_string()) {
        error = "Pluto portable asset metadata is incomplete";
        return std::nullopt;
    }

    ReleaseAsset asset{};
    asset.download_url = url_iterator->get<std::string>();
    if (!std::string_view{ asset.download_url }.starts_with(kDownloadPrefix)) {
        error = "Pluto portable asset URL is not an approved GitHub download";
        return std::nullopt;
    }
    if (!ReadAssetSize(*size_iterator, asset.size)) {
        error = "Pluto portable asset size is invalid";
        return std::nullopt;
    }
    if (!ParseDigest(digest_iterator->get_ref<const std::string&>(), asset.sha256_hex)) {
        error = "Pluto portable asset SHA-256 digest is invalid";
        return std::nullopt;
    }

    return ReleaseInfo{ .version = *version, .asset = std::move(asset) };
}

bool ShouldInstall(
    const app_version::SemanticVersion current,
    const ReleaseInfo& release) noexcept {
    return release.version > current;
}

bool IsApprovedMetadataResponseUrl(const std::string_view url) noexcept {
    return url.starts_with("https://api.github.com/");
}

bool IsApprovedAssetResponseUrl(const std::string_view url) noexcept {
    return url.starts_with(kDownloadPrefix)
        || url.starts_with("https://release-assets.githubusercontent.com/")
        || url.starts_with("https://objects.githubusercontent.com/");
}

} // namespace updater
