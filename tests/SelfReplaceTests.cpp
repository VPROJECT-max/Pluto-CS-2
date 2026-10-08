#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "updater/SelfReplace.hpp"
#include "updater/UpdatePaths.hpp"

namespace {

void WriteText(const std::filesystem::path& path, const std::string_view text) {
    std::ofstream output{ path, std::ios::binary | std::ios::trunc };
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    assert(output.good());
}

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream input{ path, std::ios::binary };
    return { std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{} };
}

updater::ReplaceOperations TestOperations(const bool launch_succeeds) {
    updater::ReplaceOperations operations;
    operations.wait_for_process = [](std::uint32_t, std::string&) { return true; };
    operations.move_replace = [](const auto& source, const auto& destination, std::string& error) {
        std::error_code code;
        std::filesystem::remove(destination, code);
        code.clear();
        std::filesystem::rename(source, destination, code);
        if (code) {
            error = code.message();
            return false;
        }
        return true;
    };
    operations.copy_replace = [](const auto& source, const auto& destination, std::string& error) {
        std::error_code code;
        std::filesystem::copy_file(source, destination,
            std::filesystem::copy_options::overwrite_existing, code);
        if (code) {
            error = code.message();
            return false;
        }
        return true;
    };
    operations.launch = [launch_succeeds](const auto&, const auto&, std::string& error) {
        if (!launch_succeeds) {
            error = "forced launch failure";
        }
        return launch_succeeds;
    };
    return operations;
}

} // namespace

int main() {
    using updater::StartupMode;

    assert(updater::IsPortableRelease(L"C:\\Tools\\Pluto-portable.exe", false));
    assert(!updater::IsPortableRelease(L"C:\\Tools\\pluto-portable.exe", false));
    assert(!updater::IsPortableRelease(L"C:\\Tools\\cs2-external-esp.exe", false));
    assert(!updater::IsPortableRelease(L"C:\\Tools\\Pluto-portable.exe", true));

    const std::vector<std::wstring_view> skip{ L"Pluto-portable.exe", L"--skip-update" };
    assert(updater::ParseStartupArguments(skip).mode == StartupMode::skip_update);
    const std::vector<std::wstring_view> malformed{ L"Pluto-portable.exe", L"--apply-update", L"x" };
    assert(updater::ParseStartupArguments(malformed).mode == StartupMode::invalid);
    const std::vector<std::wstring_view> apply{
        L"C:\\Local\\Pluto\\Updates\\2.5.1\\Pluto-portable.exe",
        L"--apply-update", L"--original", L"C:\\Tools\\Pluto-portable.exe",
        L"--rollback", L"C:\\Tools\\Pluto-portable.exe.rollback",
        L"--parent-pid", L"42"
    };
    const auto parsed_apply = updater::ParseStartupArguments(apply);
    assert(parsed_apply.mode == StartupMode::apply_update);
    assert(parsed_apply.parent_process_id == 42);

    const auto root = std::filesystem::temp_directory_path() / "pluto-self-replace-tests";
    std::filesystem::remove_all(root);
    const auto local = root / "LocalAppData";
    const auto original = root / "Install" / "Pluto-portable.exe";
    std::filesystem::create_directories(original.parent_path());

    std::string error;
    const auto paths = updater::BuildUpdatePaths(original, local, "2.5.1", error);
    assert(paths);
    assert(error.empty());
    assert(paths->staged_executable.parent_path() == local / "Pluto" / "Updates" / "2.5.1");

    updater::ApplyRequest valid{
        paths->staged_executable, paths->original_executable, paths->rollback_executable,
        local / "Pluto" / "Updates", 42
    };
    assert(updater::ValidateApplyRequest(valid, error));

    auto outside = valid;
    outside.staged_executable = root / "outside.exe";
    assert(!updater::ValidateApplyRequest(outside, error));
    auto sibling = valid;
    sibling.staged_executable = local / "Pluto" / "Updates-evil" / "2.5.1" / "Pluto-portable.exe";
    assert(!updater::ValidateApplyRequest(sibling, error));
    auto traversal = valid;
    traversal.staged_executable = local / "Pluto" / "Updates" / "2.5.1" / ".." / ".." / "outside.exe";
    assert(!updater::ValidateApplyRequest(traversal, error));

    std::filesystem::create_directories(paths->staged_executable.parent_path());
    WriteText(paths->original_executable, "old");
    WriteText(paths->staged_executable, "new");
    assert(updater::ApplyVerifiedUpdate(valid, TestOperations(true), error) == 0);
    assert(ReadText(paths->original_executable) == "new");
    assert(ReadText(paths->rollback_executable) == "old");

    std::filesystem::remove(paths->rollback_executable);
    WriteText(paths->original_executable, "old-again");
    WriteText(paths->staged_executable, "new-again");
    assert(updater::ApplyVerifiedUpdate(valid, TestOperations(false), error) != 0);
    assert(ReadText(paths->original_executable) == "old-again");
    assert(!std::filesystem::exists(paths->rollback_executable));

    std::filesystem::remove_all(root);
    std::cout << "self replace tests passed\n";
    return 0;
}
