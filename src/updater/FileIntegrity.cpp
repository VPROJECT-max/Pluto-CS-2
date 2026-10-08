#include "FileIntegrity.hpp"

#include <Windows.h>
#include <bcrypt.h>

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <vector>

#pragma comment(lib, "Bcrypt.lib")

namespace updater {
namespace {

class AlgorithmHandle {
public:
    ~AlgorithmHandle() {
        if (value_ != nullptr) {
            BCryptCloseAlgorithmProvider(value_, 0);
        }
    }

    AlgorithmHandle(const AlgorithmHandle&) = delete;
    AlgorithmHandle& operator=(const AlgorithmHandle&) = delete;
    AlgorithmHandle() = default;

    [[nodiscard]] BCRYPT_ALG_HANDLE* put() noexcept { return &value_; }
    [[nodiscard]] BCRYPT_ALG_HANDLE get() const noexcept { return value_; }

private:
    BCRYPT_ALG_HANDLE value_{};
};

class HashHandle {
public:
    ~HashHandle() {
        if (value_ != nullptr) {
            BCryptDestroyHash(value_);
        }
    }

    HashHandle(const HashHandle&) = delete;
    HashHandle& operator=(const HashHandle&) = delete;
    HashHandle() = default;

    [[nodiscard]] BCRYPT_HASH_HANDLE* put() noexcept { return &value_; }
    [[nodiscard]] BCRYPT_HASH_HANDLE get() const noexcept { return value_; }

private:
    BCRYPT_HASH_HANDLE value_{};
};

[[nodiscard]] bool ReadProperty(
    BCRYPT_ALG_HANDLE algorithm,
    const wchar_t* name,
    DWORD& value,
    std::string& error) {
    DWORD copied{};
    const auto status = BCryptGetProperty(
        algorithm,
        name,
        reinterpret_cast<PUCHAR>(&value),
        sizeof(value),
        &copied,
        0);
    if (!BCRYPT_SUCCESS(status) || copied != sizeof(value)) {
        error = "Unable to query the SHA-256 provider";
        return false;
    }
    return true;
}

} // namespace

std::optional<std::string> Sha256File(const std::filesystem::path& path, std::string& error) {
    error.clear();
    std::ifstream input{ path, std::ios::binary };
    if (!input.is_open()) {
        error = "Unable to open file for SHA-256 verification";
        return std::nullopt;
    }

    AlgorithmHandle algorithm;
    auto status = BCryptOpenAlgorithmProvider(algorithm.put(), BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        error = "Unable to initialize the SHA-256 provider";
        return std::nullopt;
    }

    DWORD object_size{};
    DWORD digest_size{};
    if (!ReadProperty(algorithm.get(), BCRYPT_OBJECT_LENGTH, object_size, error)
        || !ReadProperty(algorithm.get(), BCRYPT_HASH_LENGTH, digest_size, error)) {
        return std::nullopt;
    }

    std::vector<UCHAR> object(object_size);
    std::vector<UCHAR> digest(digest_size);
    HashHandle hash;
    status = BCryptCreateHash(
        algorithm.get(),
        hash.put(),
        object.data(),
        static_cast<ULONG>(object.size()),
        nullptr,
        0,
        0);
    if (!BCRYPT_SUCCESS(status)) {
        error = "Unable to create the SHA-256 hash";
        return std::nullopt;
    }

    std::array<char, 64U * 1024U> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count <= 0) {
            continue;
        }
        status = BCryptHashData(
            hash.get(),
            reinterpret_cast<PUCHAR>(buffer.data()),
            static_cast<ULONG>(count),
            0);
        if (!BCRYPT_SUCCESS(status)) {
            error = "Unable to hash the complete file";
            return std::nullopt;
        }
    }
    if (!input.eof()) {
        error = "Unable to read the complete file for verification";
        return std::nullopt;
    }

    status = BCryptFinishHash(hash.get(), digest.data(), static_cast<ULONG>(digest.size()), 0);
    if (!BCRYPT_SUCCESS(status)) {
        error = "Unable to finish the SHA-256 hash";
        return std::nullopt;
    }

    std::ostringstream encoded;
    encoded << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        encoded << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return encoded.str();
}

bool VerifyFile(
    const std::filesystem::path& path,
    const std::uint64_t expected_size,
    const std::string_view expected_sha256,
    std::string& error) {
    error.clear();
    std::error_code filesystem_error;
    if (!std::filesystem::is_regular_file(path, filesystem_error) || filesystem_error) {
        error = "Downloaded file is missing or is not a regular file";
        return false;
    }
    const auto actual_size = std::filesystem::file_size(path, filesystem_error);
    if (filesystem_error) {
        error = "Unable to read downloaded file size: " + filesystem_error.message();
        return false;
    }
    if (actual_size != expected_size) {
        error = "Downloaded file size does not match the release metadata";
        return false;
    }

    const auto actual_hash = Sha256File(path, error);
    if (!actual_hash) {
        return false;
    }
    if (*actual_hash != expected_sha256) {
        error = "Downloaded file SHA-256 does not match the GitHub release digest";
        return false;
    }
    return true;
}

} // namespace updater
