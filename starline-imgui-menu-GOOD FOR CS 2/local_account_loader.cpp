#include "local_account_loader.h"

#include "imgui_text_renderer.h"
#include "loader_framework.h"
#include "local_account.hpp"
#include "menu_framework.h"

#include <Windows.h>
#include <cstring>

namespace {

starline::LocalAccount g_account{ starline::LocalAccount::DefaultPath() };
bool g_complete = false;
bool g_create_account = true;

bool Submit() {
    auto& config = Starline::Loader::g_LoaderConfig;
    std::string error;
    const bool success = g_create_account
        ? g_account.Create(config.username, config.password, error)
        : g_account.Verify(config.username, config.password, error);
    SecureZeroMemory(config.password, sizeof(config.password));
    if (!success) {
        config.loginFailed = true;
        config.errorMessage = std::move(error);
        return false;
    }
    config.loginFailed = false;
    config.errorMessage.clear();
    g_complete = true;
    return true;
}

} // namespace

namespace Starline::LocalAccountLoader {

void Initialize() {
    Loader::GUI::Initialize();
    g_complete = false;
    g_create_account = !g_account.Exists();
}

bool Render() {
    if (g_complete)
        return true;

    auto& config = Loader::g_LoaderConfig;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 window_size{ 340.0f, 300.0f };
    ImGui::SetNextWindowPos({ (display.x - window_size.x) * 0.5f, (display.y - window_size.y) * 0.5f });
    ImGui::SetNextWindowSize(window_size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::ToVec4(Colors::BackgroundDark()));
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::ToVec4(Colors::Border()));
    ImGui::Begin("##ExternalEspLocalAccount", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 window_position = ImGui::GetWindowPos();
    const char* title = g_create_account ? "create local account" : "local sign in";
    const ImVec2 title_size = g_TextRenderer.MeasureText(title, g_TextFont);
    g_TextRenderer.RenderText(draw,
        { window_position.x + (window_size.x - title_size.x) * 0.5f, window_position.y + 24.0f },
        title, Colors::TextActive(), g_TextFont);

    constexpr float content_width = 280.0f;
    constexpr float content_x = 30.0f;
    ImGui::SetCursorPos({ content_x, 66.0f });
    Loader::GUI::InputField("##username_input", "username", config.username,
        sizeof(config.username), false, &config.usernameFocused);
    ImGui::SetCursorPos({ content_x, ImGui::GetCursorPosY() + 12.0f });
    Loader::GUI::InputField("##password_input", "password", config.password,
        sizeof(config.password), true, &config.passwordFocused);
    ImGui::SetCursorPos({ content_x, ImGui::GetCursorPosY() + 14.0f });
    Loader::GUI::Checkbox("remember me", &config.rememberMe);
    ImGui::SetCursorPos({ content_x, ImGui::GetCursorPosY() + 18.0f });
    if (Loader::GUI::Button(g_create_account ? "create account" : "login", content_width))
        Submit();

    if ((config.usernameFocused || config.passwordFocused) && ImGui::IsKeyPressed(ImGuiKey_Enter))
        Submit();

    if (config.loginFailed && !config.errorMessage.empty()) {
        const ImVec2 error_size = g_TextRenderer.MeasureText(config.errorMessage, g_TextFont);
        g_TextRenderer.RenderText(draw,
            { window_position.x + (window_size.x - error_size.x) * 0.5f, window_position.y + 270.0f },
            config.errorMessage, IM_COL32(255, 100, 100, 255), g_TextFont);
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    return g_complete;
}

} // namespace Starline::LocalAccountLoader
