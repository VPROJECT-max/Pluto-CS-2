#pragma once

#include <cstdint>

struct SystemTelemetrySnapshot {
    float cpu_percent{};
    float working_set_mib{};
};

class SystemTelemetry {
public:
    SystemTelemetry() noexcept;

    [[nodiscard]] SystemTelemetrySnapshot Sample() noexcept;

private:
    std::uint64_t previous_process_ticks_{};
    std::uint64_t previous_wall_ticks_{};
    std::uint32_t processor_count_{ 1 };
    bool has_previous_sample_{};
};
