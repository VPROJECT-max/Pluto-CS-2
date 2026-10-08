#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace updater {

[[nodiscard]] std::optional<std::string> Sha256File(
    const std::filesystem::path& path,
    std::string& error);

[[nodiscard]] bool VerifyFile(
    const std::filesystem::path& path,
    std::uint64_t expected_size,
    std::string_view expected_sha256,
    std::string& error);

} // namespace updater
