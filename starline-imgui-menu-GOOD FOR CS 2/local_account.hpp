#pragma once

#include <filesystem>
#include <string>

namespace starline {

class LocalAccount final {
public:
    explicit LocalAccount(std::filesystem::path path);

    [[nodiscard]] static std::filesystem::path DefaultPath();
    [[nodiscard]] bool Exists() const;
    [[nodiscard]] bool Create(const std::string& username, const std::string& password, std::string& error) const;
    [[nodiscard]] bool Verify(const std::string& username, const std::string& password, std::string& error) const;

private:
    std::filesystem::path path_;
};

} // namespace starline
