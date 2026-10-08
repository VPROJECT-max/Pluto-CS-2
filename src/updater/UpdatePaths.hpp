#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace updater {

inline constexpr std::wstring_view portable_filename{ L"Pluto-portable.exe" };

enum class StartupMode {
    normal,
    skip_update,
    apply_update,
    cleanup_update,
    invalid
};

struct StartupArguments {
    StartupMode mode{ StartupMode::normal };
    std::filesystem::path executable;
    std::filesystem::path original;
    std::filesystem::path staging;
    std::filesystem::path rollback;
    std::uint32_t parent_process_id{};
    std::wstring parent_ready_event;
};

struct UpdatePaths {
    std::filesystem::path updates_root;
    std::filesystem::path version_directory;
    std::filesystem::path partial_executable;
    std::filesystem::path staged_executable;
    std::filesystem::path original_executable;
    std::filesystem::path rollback_executable;
};

[[nodiscard]] bool IsPortableRelease(const std::filesystem::path& module_path, bool debug_build);
[[nodiscard]] StartupArguments ParseStartupArguments(std::span<const std::wstring_view> arguments);
[[nodiscard]] std::optional<UpdatePaths> BuildUpdatePaths(
    const std::filesystem::path& module_path,
    const std::filesystem::path& local_app_data,
    std::string_view version,
    std::string& error);
[[nodiscard]] bool IsPathContainedBy(
    const std::filesystem::path& candidate,
    const std::filesystem::path& root);

} // namespace updater
