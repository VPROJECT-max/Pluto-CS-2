#pragma once

#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")

using json = nlohmann::json;

struct HttpResult {
    long status_code{};
    CURLcode curl_code{ CURLE_OK };
    std::string error;

    [[nodiscard]] bool ok() const noexcept {
        return curl_code == CURLE_OK && status_code == 200 && error.empty();
    }
};

class HttpHelper {
public:
    [[nodiscard]] static HttpResult GetJson(
        std::string_view url,
        nlohmann::json& response,
        std::size_t max_bytes);

    [[nodiscard]] static HttpResult Download(
        std::string_view url,
        const std::filesystem::path& destination,
        std::uint64_t max_bytes);

    // Compatibility wrappers retained until the legacy updater is replaced.
    static int Get(std::string url, json& response);
    static int Post(std::string url, json body, json& response);
};
