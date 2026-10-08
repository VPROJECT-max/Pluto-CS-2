#include "local_account.hpp"

#define NOMINMAX
#include <Windows.h>
#include <dpapi.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <span>
#include <system_error>
#include <vector>

namespace {

constexpr std::array<std::uint8_t, 8> kMagic{ 'E', 'E', 'S', 'P', 'A', 'C', '0', '1' };
constexpr std::array<std::uint8_t, 24> kEntropy{
    's', 't', 'a', 'r', 'l', 'i', 'n', 'e', '-', 'l', 'o', 'c', 'a', 'l', '-',
    'a', 'c', 'c', 'o', 'u', 'n', 't', '-', '1'
};
constexpr std::size_t kMaximumFieldSize = 4096;

void AppendU32(std::vector<std::uint8_t>& destination, const std::uint32_t value) {
    destination.push_back(static_cast<std::uint8_t>(value));
    destination.push_back(static_cast<std::uint8_t>(value >> 8));
    destination.push_back(static_cast<std::uint8_t>(value >> 16));
    destination.push_back(static_cast<std::uint8_t>(value >> 24));
}

bool ReadU32(const std::vector<std::uint8_t>& source, std::size_t& cursor, std::uint32_t& value) {
    if (source.size() - cursor < sizeof(std::uint32_t))
        return false;
    value = static_cast<std::uint32_t>(source[cursor])
        | (static_cast<std::uint32_t>(source[cursor + 1]) << 8)
        | (static_cast<std::uint32_t>(source[cursor + 2]) << 16)
        | (static_cast<std::uint32_t>(source[cursor + 3]) << 24);
    cursor += sizeof(std::uint32_t);
    return true;
}

DATA_BLOB Blob(std::span<const std::uint8_t> bytes) {
    DATA_BLOB blob{};
    blob.cbData = static_cast<DWORD>(bytes.size());
    blob.pbData = const_cast<BYTE*>(bytes.data());
    return blob;
}

bool Protect(const std::string& password, std::vector<std::uint8_t>& protected_data, std::string& error) {
    const auto password_bytes = std::span{
        reinterpret_cast<const std::uint8_t*>(password.data()), password.size()
    };
    DATA_BLOB input = Blob(password_bytes);
    DATA_BLOB entropy = Blob(kEntropy);
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"External ESP local account", &entropy, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        error = "Windows could not protect the local account";
        return false;
    }

    protected_data.assign(output.pbData, output.pbData + output.cbData);
    LocalFree(output.pbData);
    return true;
}

bool Unprotect(std::span<const std::uint8_t> protected_data, std::string& password, std::string& error) {
    DATA_BLOB input = Blob(protected_data);
    DATA_BLOB entropy = Blob(kEntropy);
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, &entropy, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        error = "The local account file is invalid for this Windows user";
        return false;
    }

    password.assign(reinterpret_cast<const char*>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return true;
}

bool ConstantTimeEqual(const std::string& left, const std::string& right) {
    const std::size_t count = (std::max)(left.size(), right.size());
    unsigned char difference = static_cast<unsigned char>(left.size() ^ right.size());
    for (std::size_t index = 0; index < count; ++index) {
        const unsigned char lhs = index < left.size() ? static_cast<unsigned char>(left[index]) : 0;
        const unsigned char rhs = index < right.size() ? static_cast<unsigned char>(right[index]) : 0;
        difference |= static_cast<unsigned char>(lhs ^ rhs);
    }
    return difference == 0;
}

} // namespace

namespace starline {

LocalAccount::LocalAccount(std::filesystem::path path) : path_{ std::move(path) } {}

std::filesystem::path LocalAccount::DefaultPath() {
    std::array<wchar_t, 32768> buffer{};
    const DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0 || size >= buffer.size())
        return std::filesystem::current_path() / "account.dat";
    return std::filesystem::path{ buffer.data() } / "ExternalESP" / "account.dat";
}

bool LocalAccount::Exists() const {
    std::error_code error;
    return std::filesystem::is_regular_file(path_, error);
}

bool LocalAccount::Create(const std::string& username, const std::string& password, std::string& error) const {
    error.clear();
    if (username.empty() || password.empty()) {
        error = "Username and password are required";
        return false;
    }
    if (username.size() > 63 || password.size() > kMaximumFieldSize) {
        error = "Username or password is too long";
        return false;
    }

    std::vector<std::uint8_t> protected_password;
    if (!Protect(password, protected_password, error))
        return false;

    std::vector<std::uint8_t> document;
    document.reserve(kMagic.size() + 8 + username.size() + protected_password.size());
    document.insert(document.end(), kMagic.begin(), kMagic.end());
    AppendU32(document, static_cast<std::uint32_t>(username.size()));
    AppendU32(document, static_cast<std::uint32_t>(protected_password.size()));
    document.insert(document.end(), username.begin(), username.end());
    document.insert(document.end(), protected_password.begin(), protected_password.end());

    std::error_code filesystem_error;
    std::filesystem::create_directories(path_.parent_path(), filesystem_error);
    if (filesystem_error) {
        error = "Could not create the local account folder";
        return false;
    }

    const auto temporary = path_.wstring() + L".tmp";
    {
        std::ofstream output{ temporary, std::ios::binary | std::ios::trunc };
        if (!output) {
            error = "Could not write the local account file";
            return false;
        }
        output.write(reinterpret_cast<const char*>(document.data()), static_cast<std::streamsize>(document.size()));
        if (!output) {
            error = "Could not finish writing the local account file";
            return false;
        }
    }

    if (!MoveFileExW(temporary.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary.c_str());
        error = "Could not finalize the local account file";
        return false;
    }
    return true;
}

bool LocalAccount::Verify(const std::string& username, const std::string& password, std::string& error) const {
    error.clear();
    std::ifstream input{ path_, std::ios::binary };
    if (!input) {
        error = "Local account not found";
        return false;
    }
    const std::vector<std::uint8_t> document{
        std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{}
    };
    if (document.size() < kMagic.size() + 8 || !std::equal(kMagic.begin(), kMagic.end(), document.begin())) {
        error = "Local account file is invalid";
        return false;
    }

    std::size_t cursor = kMagic.size();
    std::uint32_t username_size{};
    std::uint32_t protected_size{};
    if (!ReadU32(document, cursor, username_size) || !ReadU32(document, cursor, protected_size)
        || username_size == 0 || username_size > 63 || protected_size == 0 || protected_size > kMaximumFieldSize
        || document.size() - cursor != static_cast<std::size_t>(username_size) + protected_size) {
        error = "Local account file is invalid";
        return false;
    }

    const std::string stored_username{
        reinterpret_cast<const char*>(document.data() + cursor), username_size
    };
    cursor += username_size;
    std::string stored_password;
    if (!Unprotect(std::span{ document.data() + cursor, static_cast<std::size_t>(protected_size) }, stored_password, error))
        return false;

    const bool matches = ConstantTimeEqual(stored_username, username)
        && ConstantTimeEqual(stored_password, password);
    SecureZeroMemory(stored_password.data(), stored_password.size());
    if (!matches)
        error = "Username or password is incorrect";
    return matches;
}

} // namespace starline
