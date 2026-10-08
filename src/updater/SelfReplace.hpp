#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace updater {

struct ApplyRequest {
    std::filesystem::path staged_executable;
    std::filesystem::path original_executable;
    std::filesystem::path rollback_executable;
    std::filesystem::path updates_root;
    std::uint32_t parent_process_id{};
    std::wstring parent_ready_event;
};

struct CleanupRequest {
    std::filesystem::path staged_executable;
    std::filesystem::path original_executable;
    std::filesystem::path rollback_executable;
    std::filesystem::path updates_root;
    std::uint32_t parent_process_id{};
    std::wstring parent_ready_event;
};

struct ReplaceOperations {
    std::function<bool(std::uint32_t, const std::filesystem::path&, std::wstring_view, std::string&)>
        wait_for_matching_process;
    std::function<bool(const std::filesystem::path&, const std::filesystem::path&, std::string&)> move_replace;
    std::function<bool(const std::filesystem::path&, const std::filesystem::path&, std::string&)> copy_replace;
    std::function<bool(const std::filesystem::path&, std::span<const std::wstring>, std::string&)> launch;
};

[[nodiscard]] bool ValidateApplyRequest(const ApplyRequest& request, std::string& error);
[[nodiscard]] bool ValidateCleanupRequest(const CleanupRequest& request, std::string& error);
[[nodiscard]] ReplaceOperations DefaultReplaceOperations();
[[nodiscard]] int ApplyVerifiedUpdate(
    const ApplyRequest& request,
    const ReplaceOperations& operations,
    std::string& error);
[[nodiscard]] int ApplyVerifiedUpdate(const ApplyRequest& request, std::string& error);
[[nodiscard]] bool CleanupUpdate(const CleanupRequest& request, std::string& error);

} // namespace updater
