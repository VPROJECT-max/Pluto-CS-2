#pragma once

#include <span>
#include <string_view>

namespace updater {

enum class StartupAction {
    continue_launch,
    exit_success,
    exit_failure
};

class Updater final {
public:
    [[nodiscard]] static StartupAction ProcessStartup(
        std::span<const std::wstring_view> arguments);
};

} // namespace updater
