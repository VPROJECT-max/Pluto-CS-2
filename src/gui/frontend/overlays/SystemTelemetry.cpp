#include "SystemTelemetry.hpp"

#include <Windows.h>
#include <Psapi.h>

#include <algorithm>

namespace {

[[nodiscard]] std::uint64_t FileTimeTicks(const FILETIME value) noexcept {
    ULARGE_INTEGER ticks{};
    ticks.LowPart = value.dwLowDateTime;
    ticks.HighPart = value.dwHighDateTime;
    return ticks.QuadPart;
}

} // namespace

SystemTelemetry::SystemTelemetry() noexcept {
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    processor_count_ = (std::max)(1U, static_cast<std::uint32_t>(info.dwNumberOfProcessors));
}

SystemTelemetrySnapshot SystemTelemetry::Sample() noexcept {
    FILETIME creation{};
    FILETIME exit{};
    FILETIME kernel{};
    FILETIME user{};
    FILETIME wall{};
    GetSystemTimeAsFileTime(&wall);

    float cpu_percent = 0.0f;
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        const std::uint64_t process_ticks = FileTimeTicks(kernel) + FileTimeTicks(user);
        const std::uint64_t wall_ticks = FileTimeTicks(wall);
        if (has_previous_sample_ && wall_ticks > previous_wall_ticks_) {
            const double process_delta = static_cast<double>(process_ticks - previous_process_ticks_);
            const double wall_delta = static_cast<double>(wall_ticks - previous_wall_ticks_);
            const double normalized = (process_delta / wall_delta) * 100.0 / processor_count_;
            cpu_percent = static_cast<float>(std::clamp(normalized, 0.0, 100.0));
        }
        previous_process_ticks_ = process_ticks;
        previous_wall_ticks_ = wall_ticks;
        has_previous_sample_ = true;
    }

    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    float working_set_mib = 0.0f;
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters))) {
        constexpr double bytes_per_mib = 1024.0 * 1024.0;
        working_set_mib = static_cast<float>(static_cast<double>(counters.WorkingSetSize) / bytes_per_mib);
    }

    return { .cpu_percent = cpu_percent, .working_set_mib = working_set_mib };
}
