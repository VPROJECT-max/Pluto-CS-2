#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <Windows.h>

#include "updater/FileIntegrity.hpp"
#include "updater/http/HttpHelper.hpp"

namespace {

void WriteBytes(const std::filesystem::path& path, const std::string_view bytes) {
    std::ofstream output{ path, std::ios::binary | std::ios::trunc };
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    assert(output.good());
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path) {
    std::ifstream input{ path, std::ios::binary };
    return { std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{} };
}

} // namespace

int main(int argc, char* argv[]) {
    const auto root = std::filesystem::temp_directory_path()
        / ("pluto-updater-io-tests-" + std::to_string(GetCurrentProcessId()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto payload = root / "payload.bin";
    WriteBytes(payload, "abc");
    constexpr std::string_view expected_hash{
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
    };
    std::string error;
    assert(updater::Sha256File(payload, error) == expected_hash);
    assert(error.empty());
    assert(updater::VerifyFile(payload, 3, expected_hash, error));
    assert(!updater::VerifyFile(payload, 4, expected_hash, error));
    assert(!error.empty());
    assert(!updater::VerifyFile(payload, 3,
        "aa7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", error));
    assert(!error.empty());

    if (argc == 2) {
        const std::string base_url = argv[1];
        nlohmann::json document;

        auto result = HttpHelper::GetJson(base_url + "/json", document, 128);
        assert(result.ok());
        assert(document.at("ok").get<bool>());

        document.clear();
        result = HttpHelper::GetJson(base_url + "/redirect", document, 128);
        assert(result.ok());
        assert(document.at("ok").get<bool>());

        document.clear();
        result = HttpHelper::GetJson(base_url + "/oversized", document, 64);
        assert(!result.ok());
        assert(!result.error.empty());

        document.clear();
        result = HttpHelper::GetJson(base_url + "/missing", document, 128);
        assert(result.status_code == 404);
        assert(!result.ok());

        const auto binary = root / "download.part";
        result = HttpHelper::Download(base_url + "/binary", binary, 64);
        assert(result.ok());
        assert((ReadBytes(binary) == std::vector<std::uint8_t>{ 0, 1, 2, 3, 255 }));

        const auto partial = root / "partial.part";
        result = HttpHelper::Download(base_url + "/partial", partial, 64);
        assert(!result.ok());
        assert(!std::filesystem::exists(partial));
    }

    std::filesystem::remove_all(root);
    std::cout << "updater io tests passed\n";
    return 0;
}
