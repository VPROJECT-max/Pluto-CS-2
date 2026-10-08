#include "local_account.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

std::filesystem::path TestRoot() {
    return std::filesystem::temp_directory_path() / "external_esp_local_account_tests";
}

std::string ReadAll(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return { std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{} };
}

} // namespace

int main() {
    using starline::LocalAccount;

    const auto root = TestRoot();
    const auto account_path = root / "account.dat";
    std::filesystem::remove_all(root);

    LocalAccount account{ account_path };
    assert(!account.Exists());

    std::string error;
    assert(!account.Create("", "password", error));
    assert(!error.empty());
    assert(!account.Create("tester", "", error));
    assert(!error.empty());

    assert(account.Create("tester", "correct horse battery staple", error));
    assert(error.empty());
    assert(account.Exists());
    assert(account.Verify("tester", "correct horse battery staple", error));
    assert(error.empty());
    assert(!account.Verify("other", "correct horse battery staple", error));
    assert(!account.Verify("tester", "wrong password", error));

    const std::string stored = ReadAll(account_path);
    assert(stored.find("correct horse battery staple") == std::string::npos);

    {
        std::ofstream corrupt(account_path, std::ios::binary | std::ios::trunc);
        corrupt << "broken";
    }
    assert(!account.Verify("tester", "correct horse battery staple", error));
    assert(!error.empty());

    std::filesystem::remove_all(root);
    std::cout << "local account tests passed\n";
    return 0;
}
