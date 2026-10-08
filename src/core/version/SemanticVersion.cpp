#include "SemanticVersion.hpp"

#include <limits>

namespace {

bool ParseComponent(
    const std::string_view value,
    std::size_t& cursor,
    std::uint32_t& component) {
    const std::size_t start = cursor;
    if (cursor >= value.size() || value[cursor] < '0' || value[cursor] > '9')
        return false;
    if (value[cursor] == '0' && cursor + 1 < value.size()
        && value[cursor + 1] >= '0' && value[cursor + 1] <= '9') {
        return false;
    }

    std::uint32_t parsed{};
    while (cursor < value.size() && value[cursor] >= '0' && value[cursor] <= '9') {
        const std::uint32_t digit = static_cast<std::uint32_t>(value[cursor] - '0');
        if (parsed > (std::numeric_limits<std::uint32_t>::max() - digit) / 10U)
            return false;
        parsed = parsed * 10U + digit;
        ++cursor;
    }

    if (cursor == start)
        return false;
    component = parsed;
    return true;
}

} // namespace

namespace app_version {

std::string SemanticVersion::ToString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::optional<SemanticVersion> ParseTag(const std::string_view tag) {
    if (tag.size() < 6 || tag.front() != 'v')
        return std::nullopt;

    SemanticVersion result{};
    std::size_t cursor = 1;
    if (!ParseComponent(tag, cursor, result.major)
        || cursor >= tag.size() || tag[cursor++] != '.'
        || !ParseComponent(tag, cursor, result.minor)
        || cursor >= tag.size() || tag[cursor++] != '.'
        || !ParseComponent(tag, cursor, result.patch)
        || cursor != tag.size()) {
        return std::nullopt;
    }

    return result;
}

} // namespace app_version
