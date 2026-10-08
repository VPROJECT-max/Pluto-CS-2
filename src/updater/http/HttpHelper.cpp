#include "HttpHelper.hpp"

#include "core/version/AppVersion.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <memory>
#include <system_error>

namespace {

constexpr long kConnectTimeoutMilliseconds = 5'000;
constexpr long kRequestTimeoutMilliseconds = 30'000;
constexpr long kMaximumRedirects = 3;
constexpr std::size_t kLegacyMaximumJsonBytes = 1024U * 1024U;
const std::string kUserAgent = "Pluto/" + std::string{ app_version::current_text };

struct CurlDeleter {
    void operator()(CURL* handle) const noexcept {
        if (handle != nullptr) {
            curl_easy_cleanup(handle);
        }
    }
};

using CurlHandle = std::unique_ptr<CURL, CurlDeleter>;

class CurlHeaders {
public:
    ~CurlHeaders() {
        if (headers_ != nullptr) {
            curl_slist_free_all(headers_);
        }
    }

    CurlHeaders(const CurlHeaders&) = delete;
    CurlHeaders& operator=(const CurlHeaders&) = delete;
    CurlHeaders() = default;

    [[nodiscard]] bool Append(const char* value) {
        auto* updated = curl_slist_append(headers_, value);
        if (updated == nullptr) {
            return false;
        }
        headers_ = updated;
        return true;
    }

    [[nodiscard]] curl_slist* get() const noexcept { return headers_; }

private:
    curl_slist* headers_{};
};

struct MemorySink {
    std::string bytes;
    std::size_t maximum{};
    bool exceeded{};
};

struct FileSink {
    std::ofstream stream;
    std::uint64_t maximum{};
    std::uint64_t written{};
    bool exceeded{};
    bool write_failed{};
};

[[nodiscard]] bool EnsureCurlInitialized(std::string& error) {
    static const CURLcode initialization = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (initialization == CURLE_OK) {
        return true;
    }
    error = std::string{ "curl_global_init failed: " } + curl_easy_strerror(initialization);
    return false;
}

[[nodiscard]] bool ConfigureRequest(
    CURL* curl,
    const std::string& url,
    curl_slist* headers,
    std::array<char, CURL_ERROR_SIZE>& error_buffer) {
    const char* redirect_protocols = std::string_view{ url }.starts_with("https://")
        ? "https"
        : "http,https";
    return curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_MAXREDIRS, kMaximumRedirects) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, kConnectTimeoutMilliseconds) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, kRequestTimeoutMilliseconds) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent.c_str()) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https") == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, redirect_protocols) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer.data()) == CURLE_OK;
}

[[nodiscard]] bool BuildGitHubHeaders(
    CurlHeaders& headers,
    const bool json_request,
    std::string& error) {
    const bool complete = headers.Append(json_request
        ? "Accept: application/vnd.github+json"
        : "Accept: application/octet-stream")
        && headers.Append("X-GitHub-Api-Version: 2022-11-28");
    if (!complete) {
        error = "Unable to allocate HTTP request headers";
    }
    return complete;
}

std::size_t WriteMemory(char* data, const std::size_t size, const std::size_t count, void* context) {
    auto& sink = *static_cast<MemorySink*>(context);
    if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) {
        sink.exceeded = true;
        return 0;
    }
    const auto byte_count = size * count;
    if (sink.bytes.size() > sink.maximum || byte_count > sink.maximum - sink.bytes.size()) {
        sink.exceeded = true;
        return 0;
    }
    sink.bytes.append(data, byte_count);
    return byte_count;
}

std::size_t WriteDownload(char* data, const std::size_t size, const std::size_t count, void* context) {
    auto& sink = *static_cast<FileSink*>(context);
    if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) {
        sink.exceeded = true;
        return 0;
    }
    const auto byte_count = size * count;
    if (sink.written > sink.maximum || byte_count > sink.maximum - sink.written) {
        sink.exceeded = true;
        return 0;
    }
    sink.stream.write(data, static_cast<std::streamsize>(byte_count));
    if (!sink.stream.good()) {
        sink.write_failed = true;
        return 0;
    }
    sink.written += byte_count;
    return byte_count;
}

void FinishResult(CURL* curl, HttpResult& result, const std::array<char, CURL_ERROR_SIZE>& buffer) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);
    char* effective_url = nullptr;
    if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url) == CURLE_OK
        && effective_url != nullptr) {
        result.effective_url = effective_url;
    }
    if (result.curl_code != CURLE_OK) {
        result.error = buffer.front() != '\0' ? buffer.data() : curl_easy_strerror(result.curl_code);
    }
}

} // namespace

HttpResult HttpHelper::GetJson(
    const std::string_view url,
    nlohmann::json& response,
    const std::size_t max_bytes) {
    HttpResult result;
    if (max_bytes == 0) {
        result.error = "Maximum response size must be greater than zero";
        return result;
    }
    if (!EnsureCurlInitialized(result.error)) {
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }

    CurlHandle curl{ curl_easy_init() };
    if (!curl) {
        result.curl_code = CURLE_FAILED_INIT;
        result.error = "curl_easy_init failed";
        return result;
    }
    CurlHeaders headers;
    if (!BuildGitHubHeaders(headers, true, result.error)) {
        result.curl_code = CURLE_OUT_OF_MEMORY;
        return result;
    }

    MemorySink sink{ {}, max_bytes, false };
    std::array<char, CURL_ERROR_SIZE> error_buffer{};
    const std::string owned_url{ url };
    if (!ConfigureRequest(curl.get(), owned_url, headers.get(), error_buffer)
        || curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteMemory) != CURLE_OK
        || curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &sink) != CURLE_OK) {
        result.curl_code = CURLE_FAILED_INIT;
        result.error = "Unable to configure HTTP request";
        return result;
    }

    result.curl_code = curl_easy_perform(curl.get());
    FinishResult(curl.get(), result, error_buffer);
    if (sink.exceeded) {
        result.error = "HTTP response exceeded the configured size limit";
        return result;
    }
    if (result.curl_code != CURLE_OK) {
        return result;
    }
    if (result.status_code != 200) {
        result.error = "HTTP request returned status " + std::to_string(result.status_code);
        return result;
    }

    try {
        response = nlohmann::json::parse(sink.bytes);
    } catch (const nlohmann::json::exception& exception) {
        result.error = std::string{ "Invalid JSON response: " } + exception.what();
    }
    return result;
}

HttpResult HttpHelper::Download(
    const std::string_view url,
    const std::filesystem::path& destination,
    const std::uint64_t max_bytes) {
    HttpResult result;
    std::error_code filesystem_error;
    std::filesystem::remove(destination, filesystem_error);
    filesystem_error.clear();
    if (max_bytes == 0) {
        result.error = "Maximum download size must be greater than zero";
        return result;
    }
    if (!destination.parent_path().empty()) {
        std::filesystem::create_directories(destination.parent_path(), filesystem_error);
        if (filesystem_error) {
            result.error = "Unable to create download directory: " + filesystem_error.message();
            return result;
        }
    }
    if (!EnsureCurlInitialized(result.error)) {
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }

    CurlHandle curl{ curl_easy_init() };
    if (!curl) {
        result.curl_code = CURLE_FAILED_INIT;
        result.error = "curl_easy_init failed";
        return result;
    }
    CurlHeaders headers;
    if (!BuildGitHubHeaders(headers, false, result.error)) {
        result.curl_code = CURLE_OUT_OF_MEMORY;
        return result;
    }

    FileSink sink{ std::ofstream{ destination, std::ios::binary | std::ios::trunc }, max_bytes };
    if (!sink.stream.is_open()) {
        result.error = "Unable to open the download destination";
        return result;
    }

    std::array<char, CURL_ERROR_SIZE> error_buffer{};
    const std::string owned_url{ url };
    if (!ConfigureRequest(curl.get(), owned_url, headers.get(), error_buffer)
        || curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteDownload) != CURLE_OK
        || curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &sink) != CURLE_OK) {
        result.curl_code = CURLE_FAILED_INIT;
        result.error = "Unable to configure download request";
    } else {
        result.curl_code = curl_easy_perform(curl.get());
        FinishResult(curl.get(), result, error_buffer);
    }
    sink.stream.close();

    if (sink.exceeded) {
        result.error = "Download exceeded the configured size limit";
    } else if (sink.write_failed) {
        result.error = "Unable to write the complete download";
    } else if (result.curl_code == CURLE_OK && result.status_code != 200) {
        result.error = "Download returned HTTP status " + std::to_string(result.status_code);
    }

    if (!result.ok()) {
        filesystem_error.clear();
        std::filesystem::remove(destination, filesystem_error);
    }
    return result;
}

int HttpHelper::Get(std::string url, json& response) {
    const auto result = GetJson(url, response, kLegacyMaximumJsonBytes);
    if (result.curl_code != CURLE_OK) {
        return -1;
    }
    if (!result.error.empty() && result.status_code == 200) {
        return -2;
    }
    return static_cast<int>(result.status_code);
}

int HttpHelper::Post(std::string, json, json&) {
    return -1;
}
