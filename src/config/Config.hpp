#pragma once

#include "Current.hpp"

using json = nlohmann::json;

class Config {
public:
    ~Config() = default;
    Config(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(const Config&) = delete;
    Config& operator=(Config&&) = delete;

    static bool Read();
    static bool Write();
    static bool ResetEsp();
    static bool ResetWorld();
    static bool ResetSettings();
    static bool ResetAll();
    static const std::string& LastStatus();
private:
    Config() {};

    static Config& GetInstance()
    {
        static Config i{};
        return i;
    }

    bool ReadImpl();
    bool WriteImpl();
	std::string last_status{ "Configuration not loaded" };

    static color_t JsonToColor(const json& parent, const std::string& key, const color_t& def);
    static void ColorToJson(json& parent, const std::string& key, const color_t& color);
    static void Vec2ToJson(json& parent, const std::string& key, const Vec2_t& vec);
    static Vec2_t JsonToVec2(const json& parent, const std::string& key, const Vec2_t& def);
};
