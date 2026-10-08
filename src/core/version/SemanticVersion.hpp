#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace app_version {

struct SemanticVersion final {
    std::uint32_t major{};
    std::uint32_t minor{};
    std::uint32_t patch{};

    auto operator<=>(const SemanticVersion&) const = default;

    [[nodiscard]] std::string ToString() const;
};

[[nodiscard]] std::optional<SemanticVersion> ParseTag(std::string_view tag);

} // namespace app_version
