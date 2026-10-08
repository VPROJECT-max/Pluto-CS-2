#include "UpdatePaths.hpp"

#include <algorithm>
#include <cwctype>
#include <limits>

namespace updater {
namespace {

[[nodiscard]] bool EqualPathComponent(const std::filesystem::path& left, const std::filesystem::path& right) {
    const auto& left_text = left.native();
    const auto& right_text = right.native();
    return left_text.size() == right_text.size()
        && std::equal(left_text.begin(), left_text.end(), right_text.begin(), [](const wchar_t a, const wchar_t b) {
            return std::towlower(a) == std::towlower(b);
        });
}

[[nodiscard]] bool ParseProcessId(const std::wstring_view text, std::uint32_t& value) {
    if (text.empty()) {
        return false;
    }
    std::uint64_t parsed{};
    for (const wchar_t character : text) {
        if (character < L'0' || character > L'9') {
            return false;
        }
        parsed = parsed * 10U + static_cast<unsigned int>(character - L'0');
        if (parsed > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
    }
    if (parsed == 0) {
        return false;
    }
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

[[nodiscard]] bool IsSafeVersionComponent(const std::string_view version) {
    if (version.empty() || version == "." || version == "..") {
        return false;
    }
    unsigned int dots{};
    bool previous_was_dot = true;
    for (const char character : version) {
        if (character == '.') {
            if (previous_was_dot) {
                return false;
            }
            previous_was_dot = true;
            ++dots;
        } else if (character >= '0' && character <= '9') {
            previous_was_dot = false;
        } else {
            return false;
        }
    }
    return dots == 2 && !previous_was_dot;
}

[[nodiscard]] std::filesystem::path AbsoluteNormalized(const std::filesystem::path& value) {
    std::error_code error;
    auto canonical = std::filesystem::weakly_canonical(value, error);
    if (!error) {
        return canonical.lexically_normal();
    }
    error.clear();
    auto absolute = std::filesystem::absolute(value, error);
    if (error) {
        return value.lexically_normal();
    }
    return absolute.lexically_normal();
}

} // namespace

bool IsPortableRelease(const std::filesystem::path& module_path, const bool debug_build) {
    return !debug_build && module_path.filename().native() == portable_filename;
}

StartupArguments ParseStartupArguments(const std::span<const std::wstring_view> arguments) {
    StartupArguments parsed;
    if (arguments.empty() || arguments.front().empty()) {
        parsed.mode = StartupMode::invalid;
        return parsed;
    }
    parsed.executable = arguments.front();
    if (arguments.size() == 1) {
        return parsed;
    }
    if (arguments.size() == 2 && arguments[1] == L"--skip-update") {
        parsed.mode = StartupMode::skip_update;
        return parsed;
    }
    if (arguments.size() == 8
        && arguments[1] == L"--apply-update"
        && arguments[2] == L"--original"
        && arguments[4] == L"--rollback"
        && arguments[6] == L"--parent-pid"
        && ParseProcessId(arguments[7], parsed.parent_process_id)) {
        parsed.mode = StartupMode::apply_update;
        parsed.staging = parsed.executable;
        parsed.original = arguments[3];
        parsed.rollback = arguments[5];
        return parsed;
    }
    if (arguments.size() == 8
        && arguments[1] == L"--cleanup-update"
        && arguments[2] == L"--staging"
        && arguments[4] == L"--rollback"
        && arguments[6] == L"--parent-pid"
        && ParseProcessId(arguments[7], parsed.parent_process_id)) {
        parsed.mode = StartupMode::cleanup_update;
        parsed.original = parsed.executable;
        parsed.staging = arguments[3];
        parsed.rollback = arguments[5];
        return parsed;
    }
    parsed.mode = StartupMode::invalid;
    return parsed;
}

std::optional<UpdatePaths> BuildUpdatePaths(
    const std::filesystem::path& module_path,
    const std::filesystem::path& local_app_data,
    const std::string_view version,
    std::string& error) {
    error.clear();
    if (module_path.empty() || local_app_data.empty()) {
        error = "Update paths require executable and local application-data locations";
        return std::nullopt;
    }
    if (!IsSafeVersionComponent(version)) {
        error = "Update version is not a safe directory component";
        return std::nullopt;
    }

    UpdatePaths paths;
    paths.updates_root = AbsoluteNormalized(local_app_data / "Pluto" / "Updates");
    paths.version_directory = paths.updates_root / version;
    paths.partial_executable = paths.version_directory / "Pluto-portable.exe.part";
    paths.staged_executable = paths.version_directory / portable_filename;
    paths.original_executable = AbsoluteNormalized(module_path);
    paths.rollback_executable = std::filesystem::path{ paths.original_executable.native() + L".rollback" };
    if (!IsPathContainedBy(paths.staged_executable, paths.updates_root)
        || !IsPathContainedBy(paths.partial_executable, paths.updates_root)) {
        error = "Generated update paths escaped the updates directory";
        return std::nullopt;
    }
    return paths;
}

bool IsPathContainedBy(
    const std::filesystem::path& candidate,
    const std::filesystem::path& root) {
    const auto normalized_candidate = AbsoluteNormalized(candidate);
    const auto normalized_root = AbsoluteNormalized(root);
    auto candidate_part = normalized_candidate.begin();
    auto root_part = normalized_root.begin();
    for (; root_part != normalized_root.end(); ++root_part, ++candidate_part) {
        if (candidate_part == normalized_candidate.end() || !EqualPathComponent(*candidate_part, *root_part)) {
            return false;
        }
    }
    return candidate_part != normalized_candidate.end();
}

} // namespace updater
