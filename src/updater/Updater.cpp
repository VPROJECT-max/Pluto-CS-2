#include "Updater.hpp"

#include "FileIntegrity.hpp"
#include "ReleaseModel.hpp"
#include "SelfReplace.hpp"
#include "UpdatePaths.hpp"
#include "core/version/AppVersion.hpp"
#include "http/HttpHelper.hpp"

#include <Windows.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace updater {
namespace {

constexpr std::string_view kLatestReleaseUrl{
    "https://api.github.com/repos/VPROJECT-max/Pluto-CS-2/releases/latest"
};
constexpr std::size_t kMaximumReleaseMetadataBytes = 256U * 1024U;

[[nodiscard]] bool IsDebugBuild() noexcept {
#ifdef _DEBUG
    return true;
#else
    return false;
#endif
}

[[nodiscard]] std::optional<std::filesystem::path> ModulePath(std::string& error) {
    std::wstring buffer(32'768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        error = "Unable to determine the Pluto executable path";
        return std::nullopt;
    }
    buffer.resize(length);
    return std::filesystem::path{ buffer };
}

[[nodiscard]] std::optional<std::filesystem::path> LocalAppData(std::string& error) {
    const DWORD required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
    if (required <= 1) {
        error = "LOCALAPPDATA is unavailable";
        return std::nullopt;
    }
    std::wstring value(required, L'\0');
    const DWORD written = GetEnvironmentVariableW(L"LOCALAPPDATA", value.data(), required);
    if (written == 0 || written >= required) {
        error = "Unable to read LOCALAPPDATA";
        return std::nullopt;
    }
    value.resize(written);
    return std::filesystem::path{ value };
}

void LogSkipped(const std::string& reason) {
    LOGF(INFO, "Pluto updater skipped: {}", reason);
}

[[nodiscard]] StartupAction RunApplyMode(
    const StartupArguments& arguments,
    const std::filesystem::path& module_path,
    const std::filesystem::path& local_app_data) {
    const ApplyRequest request{
        module_path,
        arguments.original,
        arguments.rollback,
        local_app_data / "Pluto" / "Updates",
        arguments.parent_process_id
    };
    std::string error;
    const int result = ApplyVerifiedUpdate(request, error);
    if (result != 0) {
        LOGF(WARNING, "Pluto update replacement failed: {}", error);
        return StartupAction::exit_failure;
    }
    return StartupAction::exit_success;
}

[[nodiscard]] StartupAction RunCleanupMode(
    const StartupArguments& arguments,
    const std::filesystem::path& module_path,
    const std::filesystem::path& local_app_data) {
    const CleanupRequest request{
        arguments.staging,
        module_path,
        arguments.rollback,
        local_app_data / "Pluto" / "Updates",
        arguments.parent_process_id
    };
    std::string error;
    if (!CleanupUpdate(request, error)) {
        LOGF(WARNING, "Pluto update cleanup was incomplete: {}", error);
    } else {
        LOGF(INFO, "Pluto update cleanup completed");
    }
    return StartupAction::continue_launch;
}

[[nodiscard]] StartupAction CheckForUpdate(
    const std::filesystem::path& module_path,
    const std::filesystem::path& local_app_data) {
    nlohmann::json document;
    const auto metadata = HttpHelper::GetJson(
        kLatestReleaseUrl, document, kMaximumReleaseMetadataBytes);
    if (!metadata.ok()) {
        LOGF(WARNING, "Pluto update check failed; continuing current version: {}", metadata.error);
        return StartupAction::continue_launch;
    }

    std::string error;
    const auto release = ParseLatestRelease(document, error);
    if (!release) {
        LOGF(WARNING, "Pluto release metadata was rejected; continuing current version: {}", error);
        return StartupAction::continue_launch;
    }
    if (!ShouldInstall(app_version::current, *release)) {
        LOGF(INFO, "Pluto v{} is current", app_version::current_text);
        return StartupAction::continue_launch;
    }

    const auto paths = BuildUpdatePaths(
        module_path, local_app_data, release->version.ToString(), error);
    if (!paths) {
        LOGF(WARNING, "Pluto update paths were rejected: {}", error);
        return StartupAction::continue_launch;
    }
    std::error_code filesystem_error;
    std::filesystem::create_directories(paths->version_directory, filesystem_error);
    if (filesystem_error) {
        LOGF(WARNING, "Unable to create Pluto update staging: {}", filesystem_error.message());
        return StartupAction::continue_launch;
    }

    const auto download = HttpHelper::Download(
        release->asset.download_url,
        paths->partial_executable,
        release->asset.size);
    if (!download.ok()) {
        LOGF(WARNING, "Pluto update download failed; continuing current version: {}", download.error);
        return StartupAction::continue_launch;
    }
    if (!VerifyFile(
        paths->partial_executable,
        release->asset.size,
        release->asset.sha256_hex,
        error)) {
        std::filesystem::remove(paths->partial_executable, filesystem_error);
        LOGF(WARNING, "Pluto update verification failed; downloaded file was removed: {}", error);
        return StartupAction::continue_launch;
    }

    std::filesystem::remove(paths->staged_executable, filesystem_error);
    filesystem_error.clear();
    std::filesystem::rename(paths->partial_executable, paths->staged_executable, filesystem_error);
    if (filesystem_error) {
        std::filesystem::remove(paths->partial_executable, filesystem_error);
        LOGF(WARNING, "Unable to finalize Pluto update staging");
        return StartupAction::continue_launch;
    }

    const std::vector<std::wstring> launch_arguments{
        L"--apply-update",
        L"--original", paths->original_executable.native(),
        L"--rollback", paths->rollback_executable.native(),
        L"--parent-pid", std::to_wstring(GetCurrentProcessId())
    };
    auto operations = DefaultReplaceOperations();
    if (!operations.launch(paths->staged_executable, launch_arguments, error)) {
        std::filesystem::remove(paths->staged_executable, filesystem_error);
        LOGF(WARNING, "Unable to launch staged Pluto update; continuing current version: {}", error);
        return StartupAction::continue_launch;
    }

    LOGF(INFO, "Pluto v{} is ready and will replace this process", release->version.ToString());
    return StartupAction::exit_success;
}

} // namespace

StartupAction Updater::ProcessStartup(const std::span<const std::wstring_view> arguments) {
    const auto parsed = ParseStartupArguments(arguments);
    if (parsed.mode == StartupMode::invalid) {
        LOGF(WARNING, "Invalid Pluto updater maintenance arguments");
        return StartupAction::exit_failure;
    }

    std::string error;
    const auto module_path = ModulePath(error);
    if (!module_path) {
        LOGF(WARNING, "{}; updater skipped", error);
        return parsed.mode == StartupMode::normal || parsed.mode == StartupMode::skip_update
            ? StartupAction::continue_launch
            : StartupAction::exit_failure;
    }
    const auto local_app_data = LocalAppData(error);
    if (!local_app_data) {
        LOGF(WARNING, "{}; updater skipped", error);
        return parsed.mode == StartupMode::normal || parsed.mode == StartupMode::skip_update
            ? StartupAction::continue_launch
            : StartupAction::exit_failure;
    }

    switch (parsed.mode) {
    case StartupMode::skip_update:
        LogSkipped("--skip-update requested");
        return StartupAction::continue_launch;
    case StartupMode::apply_update:
        return RunApplyMode(parsed, *module_path, *local_app_data);
    case StartupMode::cleanup_update:
        return RunCleanupMode(parsed, *module_path, *local_app_data);
    case StartupMode::normal:
        if (!IsPortableRelease(*module_path, IsDebugBuild())) {
            LogSkipped(IsDebugBuild() ? "Debug build" : "executable is not named Pluto-portable.exe");
            return StartupAction::continue_launch;
        }
        return CheckForUpdate(*module_path, *local_app_data);
    case StartupMode::invalid:
        break;
    }
    return StartupAction::exit_failure;
}

} // namespace updater
