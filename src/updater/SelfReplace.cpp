#include "SelfReplace.hpp"

#include "UpdatePaths.hpp"

#include <Windows.h>

#include <atomic>
#include <cwctype>
#include <system_error>

namespace updater {
namespace {

[[nodiscard]] std::filesystem::path ExpectedRollback(const std::filesystem::path& original) {
    return std::filesystem::path{ original.lexically_normal().native() + L".rollback" };
}

[[nodiscard]] bool EqualNormalizedPath(
    const std::filesystem::path& left,
    const std::filesystem::path& right) {
    const auto normalized_left = std::filesystem::absolute(left).lexically_normal().native();
    const auto normalized_right = std::filesystem::absolute(right).lexically_normal().native();
    return _wcsicmp(normalized_left.c_str(), normalized_right.c_str()) == 0;
}

[[nodiscard]] bool IsSafeReadyEventName(const std::wstring_view name) {
    constexpr std::wstring_view prefix{ L"Local\\PlutoUpdate-" };
    if (!name.starts_with(prefix) || name.size() <= prefix.size() || name.size() > 128) {
        return false;
    }
    for (const wchar_t character : name.substr(prefix.size())) {
        if (!std::iswalnum(character) && character != L'-') {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool ValidateCommon(
    const std::filesystem::path& staging,
    const std::filesystem::path& original,
    const std::filesystem::path& rollback,
    const std::filesystem::path& updates_root,
    const std::uint32_t parent_process_id,
    const std::wstring_view parent_ready_event,
    std::string& error) {
    error.clear();
    if (parent_process_id == 0) {
        error = "Update parent process id is invalid";
        return false;
    }
    if (!IsSafeReadyEventName(parent_ready_event)) {
        error = "Update parent-ready event is invalid";
        return false;
    }
    if (staging.filename().native() != portable_filename
        || original.filename().native() != portable_filename) {
        error = "Update executable names do not match the portable release contract";
        return false;
    }
    if (!IsPathContainedBy(staging, updates_root)) {
        error = "Staged executable is outside the Pluto updates directory";
        return false;
    }
    if (!EqualNormalizedPath(rollback, ExpectedRollback(original))) {
        error = "Rollback path does not belong to the original executable";
        return false;
    }
    if (EqualNormalizedPath(staging, original) || IsPathContainedBy(original, updates_root)) {
        error = "Original executable location is not safe for replacement";
        return false;
    }
    return true;
}

[[nodiscard]] std::wstring QuoteCommandArgument(const std::wstring_view value) {
    if (value.find_first_of(L" \t\"") == std::wstring_view::npos) {
        return std::wstring{ value };
    }
    std::wstring quoted{ L'\"' };
    std::size_t backslashes{};
    for (const wchar_t character : value) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'\"') {
            quoted.append(backslashes * 2U + 1U, L'\\');
            quoted.push_back(L'\"');
        } else {
            quoted.append(backslashes, L'\\');
            quoted.push_back(character);
        }
        backslashes = 0;
    }
    quoted.append(backslashes * 2U, L'\\');
    quoted.push_back(L'\"');
    return quoted;
}

[[nodiscard]] bool WaitForMatchingProcess(
    const std::uint32_t process_id,
    const std::filesystem::path& expected_executable,
    const std::wstring_view ready_event_name,
    std::string& error) {
    HANDLE ready_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, std::wstring{ ready_event_name }.c_str());
    if (ready_event == nullptr) {
        error = "Unable to open the Pluto parent-ready event";
        return false;
    }
    HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (process == nullptr) {
        CloseHandle(ready_event);
        error = "Unable to open the previous Pluto process";
        return false;
    }

    std::wstring image_path(32'768, L'\0');
    DWORD image_path_length = static_cast<DWORD>(image_path.size());
    if (!QueryFullProcessImageNameW(process, 0, image_path.data(), &image_path_length)) {
        CloseHandle(process);
        CloseHandle(ready_event);
        error = "Unable to identify the previous Pluto process";
        return false;
    }
    image_path.resize(image_path_length);
    if (!EqualNormalizedPath(image_path, expected_executable)) {
        CloseHandle(process);
        CloseHandle(ready_event);
        error = "Previous process executable does not match the requested Pluto target";
        return false;
    }

    if (!SetEvent(ready_event)) {
        CloseHandle(process);
        CloseHandle(ready_event);
        error = "Unable to acknowledge the verified Pluto parent process";
        return false;
    }
    CloseHandle(ready_event);

    const DWORD wait = WaitForSingleObject(process, 60'000);
    CloseHandle(process);
    if (wait != WAIT_OBJECT_0) {
        error = wait == WAIT_TIMEOUT
            ? "Timed out waiting for the previous Pluto process"
            : "Unable to wait for the previous Pluto process";
        return false;
    }
    return true;
}

[[nodiscard]] bool MoveReplace(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& error) {
    if (!MoveFileExW(
        source.c_str(),
        destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Unable to move an update file (Win32 " + std::to_string(GetLastError()) + ")";
        return false;
    }
    return true;
}

[[nodiscard]] bool CopyReplace(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& error) {
    if (!CopyFileW(source.c_str(), destination.c_str(), FALSE)) {
        error = "Unable to copy the staged Pluto update (Win32 " + std::to_string(GetLastError()) + ")";
        return false;
    }
    return true;
}

[[nodiscard]] bool Launch(
    const std::filesystem::path& executable,
    const std::span<const std::wstring> arguments,
    std::string& error) {
    static std::atomic_uint64_t sequence{};
    const std::wstring ready_event_name = L"Local\\PlutoUpdate-"
        + std::to_wstring(GetCurrentProcessId()) + L"-"
        + std::to_wstring(GetTickCount64()) + L"-"
        + std::to_wstring(++sequence);
    HANDLE ready_event = CreateEventW(nullptr, TRUE, FALSE, ready_event_name.c_str());
    if (ready_event == nullptr) {
        error = "Unable to create the Pluto parent-ready event";
        return false;
    }

    std::wstring command = QuoteCommandArgument(executable.native());
    for (const auto& argument : arguments) {
        command.push_back(L' ');
        command += QuoteCommandArgument(argument);
    }
    command += L" --parent-ready-event ";
    command += QuoteCommandArgument(ready_event_name);

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(
        executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr,
        executable.parent_path().c_str(), &startup, &process)) {
        error = "Unable to launch the updated Pluto executable (Win32 "
            + std::to_string(GetLastError()) + ")";
        CloseHandle(ready_event);
        return false;
    }
    CloseHandle(process.hThread);

    const DWORD ready = WaitForSingleObject(ready_event, 10'000);
    CloseHandle(ready_event);
    if (ready != WAIT_OBJECT_0) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 5'000);
        CloseHandle(process.hProcess);
        error = ready == WAIT_TIMEOUT
            ? "Timed out waiting for the Pluto updater handshake"
            : "Unable to wait for the Pluto updater handshake";
        return false;
    }
    CloseHandle(process.hProcess);
    return true;
}

void RestoreRollback(
    const ApplyRequest& request,
    const ReplaceOperations& operations,
    std::string& error) {
    std::error_code remove_error;
    std::filesystem::remove(request.original_executable, remove_error);
    std::string restore_error;
    if (!operations.move_replace(request.rollback_executable, request.original_executable, restore_error)) {
        error += "; rollback failed: " + restore_error;
    }
}

} // namespace

bool ValidateApplyRequest(const ApplyRequest& request, std::string& error) {
    return ValidateCommon(
        request.staged_executable,
        request.original_executable,
        request.rollback_executable,
        request.updates_root,
        request.parent_process_id,
        request.parent_ready_event,
        error);
}

bool ValidateCleanupRequest(const CleanupRequest& request, std::string& error) {
    return ValidateCommon(
        request.staged_executable,
        request.original_executable,
        request.rollback_executable,
        request.updates_root,
        request.parent_process_id,
        request.parent_ready_event,
        error);
}

ReplaceOperations DefaultReplaceOperations() {
    return ReplaceOperations{ WaitForMatchingProcess, MoveReplace, CopyReplace, Launch };
}

int ApplyVerifiedUpdate(
    const ApplyRequest& request,
    const ReplaceOperations& operations,
    std::string& error) {
    if (!ValidateApplyRequest(request, error)) {
        return 2;
    }
    if (!operations.wait_for_matching_process
        || !operations.move_replace
        || !operations.copy_replace
        || !operations.launch) {
        error = "Update replacement operations are incomplete";
        return 3;
    }
    if (!operations.wait_for_matching_process(
        request.parent_process_id, request.original_executable, request.parent_ready_event, error)) {
        return 4;
    }
    if (!operations.move_replace(request.original_executable, request.rollback_executable, error)) {
        return 5;
    }
    if (!operations.copy_replace(request.staged_executable, request.original_executable, error)) {
        RestoreRollback(request, operations, error);
        return 6;
    }

    const std::vector<std::wstring> arguments{
        L"--cleanup-update",
        L"--staging", request.staged_executable.native(),
        L"--rollback", request.rollback_executable.native(),
        L"--parent-pid", std::to_wstring(GetCurrentProcessId())
    };
    if (!operations.launch(request.original_executable, arguments, error)) {
        RestoreRollback(request, operations, error);
        return 7;
    }
    return 0;
}

int ApplyVerifiedUpdate(const ApplyRequest& request, std::string& error) {
    return ApplyVerifiedUpdate(request, DefaultReplaceOperations(), error);
}

bool CleanupUpdate(const CleanupRequest& request, std::string& error) {
    if (!ValidateCleanupRequest(request, error)) {
        return false;
    }
    if (!WaitForMatchingProcess(
        request.parent_process_id, request.staged_executable, request.parent_ready_event, error)) {
        return false;
    }

    std::error_code filesystem_error;
    std::filesystem::remove(request.rollback_executable, filesystem_error);
    if (filesystem_error) {
        error = "Unable to remove update rollback: " + filesystem_error.message();
        return false;
    }
    std::filesystem::remove(request.staged_executable, filesystem_error);
    if (filesystem_error) {
        if (!MoveFileExW(request.staged_executable.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT)) {
            error = "Unable to remove staged updater (Win32 " + std::to_string(GetLastError()) + ")";
            return false;
        }
    }
    std::filesystem::remove(request.staged_executable.parent_path(), filesystem_error);
    return true;
}

} // namespace updater
