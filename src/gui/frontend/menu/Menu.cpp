#if 0 // Legacy bitmap-heavy menu retained for reference.
#include "Menu.hpp"

// Common includes MUST come first — brings in color_t, imgui, Logger, etc.
#include "common.hpp"

// Old project core / config / renderer
#include "config/Config.hpp"
#include "gui/renderer/Renderer.hpp"
#include "gui/renderer/window/Window.hpp"

// Images
#include "../../../assets/images/ImageLoader.hpp"
#include "../../../assets/images/AlienLogo.hpp"
#include "../../../assets/images/EspPreviewImage.hpp"

// New UI fonts & notification system
#include "Fonts.hpp"
#include "imgui_notify.h"

#include <string>
#include <vector>

// ============================================================================
//  Public static wrappers
// ============================================================================
bool Menu::Init()             { return GetInstance().InitImpl(); }
void Menu::Render()           { return GetInstance().RenderImpl(); }
void Menu::RenderStartupHelp(){ return GetInstance().RenderStartupHelpImpl(); }
ImVec2 Menu::GetPos()         { return GetInstance().pos; }
ImVec2 Menu::GetSize()        { return GetInstance().size; }

// ============================================================================
//  Init
// ============================================================================
bool Menu::InitImpl() {
    loadFont();
    loadLogo();
    loadEspPreview();
    SetupStyles();
    LOGF(INFO, "Successfully initialized menu...");
    return true;
}

// ============================================================================
//  Font loading  (Poppins + FontAwesome, no SFML)
//  The DX11 backend rebuilds the font atlas automatically on the first frame
//  after io.Fonts->Clear() is called.
// ============================================================================
void Menu::loadFont() {
    auto& io = ImGui::GetIO();

    io.Fonts->Clear();

    // -- Base font: Poppins 18 pt
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(
        (void*)poppinsFont, sizeof(poppinsFont), 18.f, &font_cfg);

    // -- Merge FontAwesome icons into the base font
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_cfg;
    icons_cfg.MergeMode            = true;
    icons_cfg.PixelSnapH           = true;
    icons_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(
        (void*)fontAwesome, sizeof(fontAwesome), 18.f, &icons_cfg, icons_ranges);

    // -- Big font: Poppins 24 pt (for logo / headings)
    ImFontConfig big_cfg;
    big_cfg.FontDataOwnedByAtlas = false;
    bigFont = io.Fonts->AddFontFromMemoryTTF(
        (void*)poppinsFont, sizeof(poppinsFont), 24.f, &big_cfg);
    io.Fonts->AddFontFromMemoryTTF(
        (void*)fontAwesome, sizeof(fontAwesome), 18.f, &icons_cfg, icons_ranges);
}

void Menu::loadLogo() {
    if (logoSRV == nullptr) {
        ImageLoader::LoadTextureFromMemory(Window::device, alien_logo_png, alien_logo_png_len, &logoSRV, &logoW, &logoH);
    }
}

void Menu::loadEspPreview() {
    if (espPreviewSRV == nullptr) {
        ImageLoader::LoadTextureFromMemory(Window::device, esp_preview_png, esp_preview_png_len, &espPreviewSRV, &espPreviewW, &espPreviewH);
    }
}

// ============================================================================
//  Style  (roundings + color palette)
// ============================================================================
void Menu::SetupStyles() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding = 6.f;
    s.ChildRounding  = 6.f;
    s.FrameRounding  = 2.f;
    s.GrabRounding   = 2.f;
    s.PopupRounding  = 2.f;

    s.ScrollbarSize   = 9.f;
    s.FramePadding    = ImVec2(6.f, 3.f);
    s.ItemSpacing     = ImVec2(4.f, 4.f);
    s.WindowBorderSize = 0.f;
    s.ChildBorderSize  = 1.f;

    setColors();
}

void Menu::setColors() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.Colors[ImGuiCol_WindowBg]          = winCol;
    s.Colors[ImGuiCol_Border]            = ImColor(0, 0, 0, 0);
    s.Colors[ImGuiCol_Button]            = bgCol;
    s.Colors[ImGuiCol_ButtonActive]      = btnActiveCol;
    s.Colors[ImGuiCol_ButtonHovered]     = btnHoverCol;
    s.Colors[ImGuiCol_FrameBg]           = bgCol;
    s.Colors[ImGuiCol_FrameBgActive]     = frameCol;
    s.Colors[ImGuiCol_FrameBgHovered]    = hoverCol;
    s.Colors[ImGuiCol_Text]              = textCol;
    s.Colors[ImGuiCol_TextDisabled]      = notSelectedTextColor;
    s.Colors[ImGuiCol_ChildBg]           = childCol;
    s.Colors[ImGuiCol_CheckMark]         = itemActiveCol;
    s.Colors[ImGuiCol_SliderGrab]        = itemCol;
    s.Colors[ImGuiCol_SliderGrabActive]  = itemActiveCol;
    s.Colors[ImGuiCol_Header]            = itemActiveCol;
    s.Colors[ImGuiCol_HeaderHovered]     = itemCol;
    s.Colors[ImGuiCol_HeaderActive]      = itemActiveCol;
    s.Colors[ImGuiCol_ResizeGrip]        = resizeGripCol;
    s.Colors[ImGuiCol_ResizeGripHovered] = resizeGripHoverCol;
    s.Colors[ImGuiCol_ResizeGripActive]  = itemActiveCol;
    s.Colors[ImGuiCol_SeparatorHovered]  = resizeGripHoverCol;
    s.Colors[ImGuiCol_SeparatorActive]   = itemActiveCol;
    s.Colors[ImGuiCol_TitleBgActive]     = itemActiveCol;
    s.Colors[ImGuiCol_PopupBg]           = childCol;
    s.Colors[ImGuiCol_ScrollbarBg]       = winCol;
    s.Colors[ImGuiCol_ScrollbarGrab]     = bgCol;
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = hoverCol;
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = itemCol;
    s.Colors[ImGuiCol_Tab]               = bgCol;
    s.Colors[ImGuiCol_TabHovered]        = btnHoverCol;
    s.Colors[ImGuiCol_TabActive]         = btnActiveCol;
}

// ============================================================================
//  Left panel
// ============================================================================
void Menu::renderPanel() {
    renderLogo();
    ImGui::Spacing();
    renderTabs();
    renderUser();
}

// -----------------------------------------------------------------------
//  Logo area (158 × 50)
// -----------------------------------------------------------------------
void Menu::renderLogo() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, childCol1);
    ImGui::BeginChild("##logo", ImVec2(158.f, 80.f), true);
    ImGui::PopStyleColor();

    if (logoSRV) {
        // Stretched manually based on user feedback
        float w = 140.f;
        float h = 75.f;
        ImGui::SetCursorPos(ImVec2((158.f - w) * 0.5f, (80.f - h) * 0.5f));
        ImGui::Image((void*)logoSRV, ImVec2(w, h));
    }

    ImGui::EndChild();
}

// -----------------------------------------------------------------------
//  Tab buttons  (158 × 220)
// -----------------------------------------------------------------------
void Menu::renderTabs() {
    ImGui::BeginChild("##tabs_panel", ImVec2(158.f, 220.f), true);

    // Search bar
    ImGui::SetNextItemWidth(140.f);
    ImGui::InputTextWithHint("##search", ICON_FA_SEARCH " Search",
                             searchBuffer, sizeof(searchBuffer));
    ImGui::Spacing();

    struct TabEntry { const char* icon; const char* label; int id; };
    static const TabEntry tabs[] = {
        { ICON_FA_CROSSHAIRS, " LegitBot", LEGITBOT },
        { ICON_FA_EYE,        " Visuals",  VISUALS  },
        { ICON_FA_COG,        " Misc",     MISC     },
        { ICON_FA_SAVE,       " Configs",  CONFIG   }
    };

    const ImVec4 transparent = ImVec4(0.f, 0.f, 0.f, 0.f);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    for (const auto& t : tabs) {
        // Skip entries that don't match the search filter
        if (searchBuffer[0] != '\0') {
            std::string label = t.label;
            std::string filter = searchBuffer;
            // Case-insensitive search
            auto toLower = [](std::string s) {
                for (auto& c : s) c = (char)tolower((unsigned char)c);
                return s;
            };
            if (toLower(label).find(toLower(filter)) == std::string::npos)
                continue;
        }

        bool isActive = (selectedTab == t.id);
        std::string btnLabel = std::string(t.icon) + t.label;

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button,
            isActive ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : transparent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            isActive ? btnHoverCol : ImVec4(btnHoverCol.x, btnHoverCol.y, btnHoverCol.z, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Text,
            isActive ? textCol : notSelectedTextColor);

        if (ImGui::Button(btnLabel.c_str(), ImVec2(140.f, 40.f)))
            selectedTab = t.id;

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }
    ImGui::PopStyleVar(); // FrameRounding

    ImGui::EndChild();
}

// -----------------------------------------------------------------------
//  User / status area  (bottom of left panel)
// -----------------------------------------------------------------------
void Menu::renderUser() {
    const float height = 60.f;
    // Push the child to the bottom of the available space
    float avail = ImGui::GetContentRegionAvail().y;
    if (avail > height)
        ImGui::Dummy(ImVec2(0.f, avail - height - ImGui::GetStyle().ItemSpacing.y));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, childCol1);
    ImGui::BeginChild("##user_panel", ImVec2(158.f, height), true);
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    ImGui::TextColored(notSelectedTextColor, ICON_FA_SHIELD " cs2-external-esp");
    ImGui::TextColored(notSelectedTextColor, "   recode  v1.0");

    ImGui::EndChild();
}

// ============================================================================
//  Right panel — LegitBot (placeholder)
// ============================================================================
void Menu::renderLegitBotTab() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild("##legitbot_wrap", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), false);
    ImGui::PopStyleColor();

    ImGui::BeginChild("##legitbot_placeholder", ImVec2(ImGuiHelper::getWidth(), 120.f), true);
    ImGui::Spacing();
    ImGui::SetCursorPosX((ImGuiHelper::getWidth() - ImGui::CalcTextSize("LegitBot coming soon...").x) * 0.5f);
    ImGui::TextColored(notSelectedTextColor, "LegitBot coming soon...");
    ImGui::EndChild();

    ImGui::EndChild();
}

// ============================================================================
//  Right panel — Visuals
//  Sub-tabs: [ESP] [World] [Other]
// ============================================================================
void Menu::renderVisualsTab() {
    const std::vector<std::string> subtabs = { "ESP", "World", "Other" };
    ImGuiHelper::drawTabHorizontally(
        "##vis_subtabs",
        ImVec2(ImGuiHelper::getWidth(), 50.f),
        subtabs,
        selectedSubTabVisuals);
    ImGui::Spacing();

    static auto cflag = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild("##vis_wrap", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), false);
    ImGui::PopStyleColor();

    // ------------------------------------------------------------------ ESP
    if (selectedSubTabVisuals == 0)
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 340.f);

        // --- Left column: Visuals + Flags
        {
            ImGui::BeginChild("##esp_vis_flags", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), true);
            
            // Use a table to organize Visuals and Flags side-by-side
            if (ImGui::BeginTable("esp_table_left", 2, ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableNextColumn();
                ImGui::Text("Visuals");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Visible Check", &cfg::esp::visible_check);
                ImGui::SetItemTooltip("Change ESP colors when the enemy is visible (spotted)");

                ImGui::Checkbox("Box", &cfg::esp::box);
                ImGui::BeginDisabled(!cfg::esp::box);
                ImGui::SameLine();
                ImGui::ColorEdit4("Team Box",   cfg::esp::colors::box_team.data(),   cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("Enemy Box",  cfg::esp::colors::box_enemy.data(),  cflag);
                if (cfg::esp::visible_check) {
                    ImGui::Indent(25.0f);
                    ImGui::ColorEdit4("Team Box Vis",   cfg::esp::colors::box_team_visible.data(),   cflag);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Enemy Box Vis",  cfg::esp::colors::box_enemy_visible.data(),  cflag);
                    ImGui::Unindent(25.0f);
                }
                ImGui::EndDisabled();

                ImGui::Checkbox("Skeleton", &cfg::esp::skeleton);
                ImGui::BeginDisabled(!cfg::esp::skeleton);
                ImGui::SameLine();
                ImGui::ColorEdit4("Team Skel",  cfg::esp::colors::skeleton_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("Enemy Skel", cfg::esp::colors::skeleton_enemy.data(), cflag);
                if (cfg::esp::visible_check) {
                    ImGui::Indent(25.0f);
                    ImGui::ColorEdit4("Team Skel Vis",  cfg::esp::colors::skeleton_team_visible.data(),  cflag);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Enemy Skel Vis", cfg::esp::colors::skeleton_enemy_visible.data(), cflag);
                    ImGui::Unindent(25.0f);
                }
                ImGui::EndDisabled();

                ImGui::Checkbox("Chams (Overlay)", &cfg::esp::chams);
                ImGui::BeginDisabled(!cfg::esp::chams);
                ImGui::SameLine();
                ImGui::ColorEdit4("Team Chams",  cfg::esp::colors::chams_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("Enemy Chams", cfg::esp::colors::chams_enemy.data(), cflag);
                if (cfg::esp::visible_check) {
                    ImGui::Indent(25.0f);
                    ImGui::ColorEdit4("Team Chams Vis",  cfg::esp::colors::chams_team_visible.data(),  cflag);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Enemy Chams Vis", cfg::esp::colors::chams_enemy_visible.data(), cflag);
                    ImGui::Unindent(25.0f);
                }
                ImGui::EndDisabled();

                ImGui::Checkbox("Head Tracker", &cfg::esp::head_tracker);
                ImGui::BeginDisabled(!cfg::esp::head_tracker);
                ImGui::SameLine();
                ImGui::ColorEdit4("Team HT",  cfg::esp::colors::tracker_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("Enemy HT", cfg::esp::colors::tracker_enemy.data(), cflag);
                if (cfg::esp::visible_check) {
                    ImGui::Indent(25.0f);
                    ImGui::ColorEdit4("Team HT Vis",  cfg::esp::colors::tracker_team_visible.data(),  cflag);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Enemy HT Vis", cfg::esp::colors::tracker_enemy_visible.data(), cflag);
                    ImGui::Unindent(25.0f);
                }
                ImGui::EndDisabled();

                ImGui::Checkbox("Tracers", &cfg::esp::tracers);
                ImGui::BeginDisabled(!cfg::esp::tracers);
                ImGui::SameLine();
                ImGui::ColorEdit4("Team Tracer",  cfg::esp::colors::tracer_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("Enemy Tracer", cfg::esp::colors::tracer_enemy.data(), cflag);
                if (cfg::esp::visible_check) {
                    ImGui::Indent(25.0f);
                    ImGui::ColorEdit4("Team Tracer Vis",  cfg::esp::colors::tracer_team_visible.data(),  cflag);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Enemy Tracer Vis", cfg::esp::colors::tracer_enemy_visible.data(), cflag);
                    ImGui::Unindent(25.0f);
                }
                ImGui::EndDisabled();

                ImGui::Checkbox("Eye Ray", &cfg::esp::eye_ray);
                ImGui::BeginDisabled(!cfg::esp::eye_ray);
                ImGui::SameLine();
                ImGui::ColorEdit4("Team Eye",  cfg::esp::colors::eye_ray_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("Enemy Eye", cfg::esp::colors::eye_ray_enemy.data(), cflag);
                if (cfg::esp::visible_check) {
                    ImGui::Indent(25.0f);
                    ImGui::ColorEdit4("Team Eye Vis",  cfg::esp::colors::eye_ray_team_visible.data(),  cflag);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Enemy Eye Vis", cfg::esp::colors::eye_ray_enemy_visible.data(), cflag);
                    ImGui::Unindent(25.0f);
                }
                ImGui::EndDisabled();
                
                ImGui::Checkbox("Spotted Only", &cfg::esp::spotted);
                ImGui::SetItemTooltip("Only show ESP if the player was spotted by you");
                ImGui::Checkbox("Show Team", &cfg::esp::team);

                ImGui::TableNextColumn();
                ImGui::Text("Info & Flags");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Health",  &cfg::esp::health);
                if (cfg::esp::health) {
                    ImGui::Indent();
                    ImGui::Checkbox("Health Number", &cfg::esp::health_number);
                    ImGui::Unindent();
                }
                ImGui::Checkbox("Armor",   &cfg::esp::armor);
                if (cfg::esp::health || cfg::esp::armor) {
                    ImGui::Indent();
                    ImGui::SliderFloat("Bar Thickness", &cfg::esp::bar_thickness, 1.0f, 6.0f, "%.1f");
                    ImGui::Unindent();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Flashed",  &cfg::esp::flags::flashed);
                ImGui::BeginDisabled(!cfg::esp::flags::flashed);
                ImGui::SameLine();
                ImGui::ColorEdit4("T Flash",  cfg::esp::colors::flags::flashed_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("E Flash",  cfg::esp::colors::flags::flashed_enemy.data(), cflag);
                ImGui::EndDisabled();

                ImGui::Checkbox("Reloading", &cfg::esp::flags::reloading);
                ImGui::BeginDisabled(!cfg::esp::flags::reloading);
                ImGui::SameLine();
                ImGui::ColorEdit4("T Reload", cfg::esp::colors::flags::reloading_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("E Reload", cfg::esp::colors::flags::reloading_enemy.data(), cflag);
                ImGui::EndDisabled();

                ImGui::Checkbox("Defusing",  &cfg::esp::flags::defusing);
                ImGui::BeginDisabled(!cfg::esp::flags::defusing);
                ImGui::SameLine();
                ImGui::ColorEdit4("T Defuse", cfg::esp::colors::flags::defusing_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("E Defuse", cfg::esp::colors::flags::defusing_enemy.data(), cflag);
                ImGui::EndDisabled();

                ImGui::Checkbox("Scoped",    &cfg::esp::flags::scoped);
                ImGui::BeginDisabled(!cfg::esp::flags::scoped);
                ImGui::SameLine();
                ImGui::ColorEdit4("T Scope",  cfg::esp::colors::flags::scoped_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("E Scope",  cfg::esp::colors::flags::scoped_enemy.data(), cflag);
                ImGui::EndDisabled();

                ImGui::Checkbox("Has C4",    &cfg::esp::flags::has_c4);
                ImGui::BeginDisabled(!cfg::esp::flags::has_c4);
                ImGui::SameLine();
                ImGui::ColorEdit4("T C4",    cfg::esp::colors::flags::c4_team.data(),  cflag);
                ImGui::SameLine();
                ImGui::ColorEdit4("E C4",    cfg::esp::colors::flags::c4_enemy.data(), cflag);
                ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Name",    &cfg::esp::flags::name);
                ImGui::SameLine(100);
                ImGui::Checkbox("Money",   &cfg::esp::flags::money);

                ImGui::Checkbox("Weapon",  &cfg::esp::flags::weapon);
                ImGui::SameLine(100);
                ImGui::Checkbox("Ammo",    &cfg::esp::flags::ammo);
                
                ImGui::Checkbox("Ping",    &cfg::esp::flags::ping);

                ImGui::EndTable();
            }

            ImGui::EndChild();
        }

        // --- Right column: ESP Preview
        ImGui::NextColumn();
        ImGui::BeginChild("##esp_right", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), true);
        ImGui::Text("Preview");
        ImGui::Separator();
        ImGui::Spacing();
        
        if (espPreviewSRV) {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            
            // Adjust scale of image if needed, assuming image is vertical (e.g. 200x400)
            float imgScale = 0.5f; 
            if (espPreviewH > 400.f) imgScale = 400.f / espPreviewH;
            ImVec2 imgSize(espPreviewW * imgScale, espPreviewH * imgScale);
            
            // Center the image horizontally
            float availWidth = ImGui::GetContentRegionAvail().x;
            float offsetX = (availWidth - imgSize.x) * 0.5f;
            if (offsetX < 0) offsetX = 0;
            
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            ImVec2 renderPos = ImGui::GetCursorScreenPos();
            ImGui::Image((void*)espPreviewSRV, imgSize);
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            // Draw simulated ESP elements over the image based on config
            // Character bounds in the image (simulated head to toe)
            float charTop = renderPos.y + imgSize.y * 0.08f;
            float charBottom = renderPos.y + imgSize.y * 0.96f;
            float charWidth = imgSize.x * 0.80f; // Wider box to cover arms
            float charLeft = renderPos.x + (imgSize.x - charWidth) * 0.5f;
            float charRight = charLeft + charWidth;
            
            // Helper color converter for config colors (which are color_t, cast to ImColor)
            auto cfgColor = [](const color_t& c) {
                return ImColor((int)(c.r * 255.f), (int)(c.g * 255.f), (int)(c.b * 255.f), (int)(c.a * 255.f));
            };
            
            auto drawScaledText = [&](ImVec2 pos, ImColor color, const char* text, float size) {
                drawList->AddText(ImGui::GetFont(), size, pos, color, text);
            };
            auto calcScaledTextSize = [&](const char* text, float size) {
                return ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
            };

            if (cfg::esp::box) {
                ImColor boxCol = cfg::esp::visible_check ? cfgColor(cfg::esp::colors::box_enemy_visible) : cfgColor(cfg::esp::colors::box_enemy);
                drawList->AddRect(ImVec2(charLeft, charTop), ImVec2(charRight, charBottom), boxCol);
            }

            if (cfg::esp::chams) {
                ImColor chamsCol = cfg::esp::visible_check ? cfgColor(cfg::esp::colors::chams_enemy_visible) : cfgColor(cfg::esp::colors::chams_enemy);
                float headY = renderPos.y + imgSize.y * 0.15f;
                float neckY = renderPos.y + imgSize.y * 0.22f;
                float midX = renderPos.x + imgSize.x * 0.5f;
                float pelvisY = renderPos.y + imgSize.y * 0.55f;
                
                // spine
                drawList->AddLine(ImVec2(midX, headY), ImVec2(midX, pelvisY), chamsCol, 6.0f);
                
                // arms
                float lShoulderX = renderPos.x + imgSize.x * 0.35f;
                float rShoulderX = renderPos.x + imgSize.x * 0.65f;
                
                float lElbowX = renderPos.x + imgSize.x * 0.22f;
                float lElbowY = renderPos.y + imgSize.y * 0.38f;
                float rElbowX = renderPos.x + imgSize.x * 0.78f;
                float rElbowY = renderPos.y + imgSize.y * 0.38f;
                
                float lHandX = renderPos.x + imgSize.x * 0.12f;
                float lHandY = renderPos.y + imgSize.y * 0.52f;
                float rHandX = renderPos.x + imgSize.x * 0.88f;
                float rHandY = renderPos.y + imgSize.y * 0.52f;

                drawList->AddLine(ImVec2(midX, neckY), ImVec2(lShoulderX, neckY), chamsCol, 6.0f);
                drawList->AddLine(ImVec2(lShoulderX, neckY), ImVec2(lElbowX, lElbowY), chamsCol, 6.0f);
                drawList->AddLine(ImVec2(lElbowX, lElbowY), ImVec2(lHandX, lHandY), chamsCol, 6.0f);
                
                drawList->AddLine(ImVec2(midX, neckY), ImVec2(rShoulderX, neckY), chamsCol, 6.0f);
                drawList->AddLine(ImVec2(rShoulderX, neckY), ImVec2(rElbowX, rElbowY), chamsCol, 6.0f);
                drawList->AddLine(ImVec2(rElbowX, rElbowY), ImVec2(rHandX, rHandY), chamsCol, 6.0f);
                
                // legs
                float lKneeX = renderPos.x + imgSize.x * 0.40f;
                float lKneeY = renderPos.y + imgSize.y * 0.72f;
                float rKneeX = renderPos.x + imgSize.x * 0.60f;
                float rKneeY = renderPos.y + imgSize.y * 0.72f;
                
                float lFootX = renderPos.x + imgSize.x * 0.38f;
                float lFootY = renderPos.y + imgSize.y * 0.94f;
                float rFootX = renderPos.x + imgSize.x * 0.62f;
                float rFootY = renderPos.y + imgSize.y * 0.94f;

                drawList->AddLine(ImVec2(midX, pelvisY), ImVec2(lKneeX, lKneeY), chamsCol, 6.0f);
                drawList->AddLine(ImVec2(lKneeX, lKneeY), ImVec2(lFootX, lFootY), chamsCol, 6.0f);
                
                drawList->AddLine(ImVec2(midX, pelvisY), ImVec2(rKneeX, rKneeY), chamsCol, 6.0f);
                drawList->AddLine(ImVec2(rKneeX, rKneeY), ImVec2(rFootX, rFootY), chamsCol, 6.0f);
            }

            if (cfg::esp::skeleton) {
                ImColor skelCol = cfg::esp::visible_check ? cfgColor(cfg::esp::colors::skeleton_enemy_visible) : cfgColor(cfg::esp::colors::skeleton_enemy);
                float headY = renderPos.y + imgSize.y * 0.15f;
                float neckY = renderPos.y + imgSize.y * 0.22f;
                float midX = renderPos.x + imgSize.x * 0.5f;
                float pelvisY = renderPos.y + imgSize.y * 0.55f;
                
                // spine
                drawList->AddLine(ImVec2(midX, headY), ImVec2(midX, pelvisY), skelCol, 1.5f);
                
                // arms
                float lShoulderX = renderPos.x + imgSize.x * 0.35f;
                float rShoulderX = renderPos.x + imgSize.x * 0.65f;
                
                float lElbowX = renderPos.x + imgSize.x * 0.22f;
                float lElbowY = renderPos.y + imgSize.y * 0.38f;
                float rElbowX = renderPos.x + imgSize.x * 0.78f;
                float rElbowY = renderPos.y + imgSize.y * 0.38f;
                
                float lHandX = renderPos.x + imgSize.x * 0.12f;
                float lHandY = renderPos.y + imgSize.y * 0.52f;
                float rHandX = renderPos.x + imgSize.x * 0.88f;
                float rHandY = renderPos.y + imgSize.y * 0.52f;

                drawList->AddLine(ImVec2(midX, neckY), ImVec2(lShoulderX, neckY), skelCol, 1.5f);
                drawList->AddLine(ImVec2(lShoulderX, neckY), ImVec2(lElbowX, lElbowY), skelCol, 1.5f);
                drawList->AddLine(ImVec2(lElbowX, lElbowY), ImVec2(lHandX, lHandY), skelCol, 1.5f);
                
                drawList->AddLine(ImVec2(midX, neckY), ImVec2(rShoulderX, neckY), skelCol, 1.5f);
                drawList->AddLine(ImVec2(rShoulderX, neckY), ImVec2(rElbowX, rElbowY), skelCol, 1.5f);
                drawList->AddLine(ImVec2(rElbowX, rElbowY), ImVec2(rHandX, rHandY), skelCol, 1.5f);
                
                // legs
                float lKneeX = renderPos.x + imgSize.x * 0.40f;
                float lKneeY = renderPos.y + imgSize.y * 0.72f;
                float rKneeX = renderPos.x + imgSize.x * 0.60f;
                float rKneeY = renderPos.y + imgSize.y * 0.72f;
                
                float lFootX = renderPos.x + imgSize.x * 0.38f;
                float lFootY = renderPos.y + imgSize.y * 0.94f;
                float rFootX = renderPos.x + imgSize.x * 0.62f;
                float rFootY = renderPos.y + imgSize.y * 0.94f;

                drawList->AddLine(ImVec2(midX, pelvisY), ImVec2(lKneeX, lKneeY), skelCol, 1.5f);
                drawList->AddLine(ImVec2(lKneeX, lKneeY), ImVec2(lFootX, lFootY), skelCol, 1.5f);
                
                drawList->AddLine(ImVec2(midX, pelvisY), ImVec2(rKneeX, rKneeY), skelCol, 1.5f);
                drawList->AddLine(ImVec2(rKneeX, rKneeY), ImVec2(rFootX, rFootY), skelCol, 1.5f);
            }
            if (cfg::esp::head_tracker) {
                float headRadius = imgSize.y * 0.05f;
                float headX = renderPos.x + imgSize.x * 0.5f;
                float headY = renderPos.y + imgSize.y * 0.15f;
                ImColor trackerCol = cfg::esp::visible_check ? cfgColor(cfg::esp::colors::tracker_enemy_visible) : cfgColor(cfg::esp::colors::tracker_enemy);
                drawList->AddCircleFilled(ImVec2(headX, headY), headRadius, trackerCol, 0);
            }
            if (cfg::esp::eye_ray) {
                float headX = renderPos.x + imgSize.x * 0.5f;
                float headY = renderPos.y + imgSize.y * 0.15f;
                float endX = headX - imgSize.x * 0.4f; // Look left
                float endY = headY;
                ImColor eyeRayCol = cfg::esp::visible_check ? cfgColor(cfg::esp::colors::eye_ray_enemy_visible) : cfgColor(cfg::esp::colors::eye_ray_enemy);
                drawList->AddLine(ImVec2(headX, headY), ImVec2(endX, endY), eyeRayCol, 1.5f);
            }
            if (cfg::esp::health) {
                float healthBarX = charLeft - (cfg::esp::bar_thickness + 4.f);
                drawList->AddRectFilled(ImVec2(healthBarX - 1.f, charTop - 1.f), ImVec2(healthBarX + cfg::esp::bar_thickness + 1.f, charBottom + 1.f), ImColor(0, 0, 0, 150));
                
                float healthFilledY = charTop + (charBottom - charTop) * 0.2f; // 80%
                float health_frac = 0.8f;
                ImU32 col_top = IM_COL32(
                    (int)(255.f - 155.f * health_frac), // R: 255 to 100
                    (int)(50.f + 205.f * health_frac),  // G: 50 to 255
                    50, 255);
                ImU32 col_bot = IM_COL32(255, 50, 50, 255);
                drawList->AddRectFilledMultiColor(ImVec2(healthBarX, healthFilledY), ImVec2(healthBarX + cfg::esp::bar_thickness, charBottom), col_top, col_top, col_bot, col_bot);
                
                if (cfg::esp::health_number) {
                    ImVec2 textSize = calcScaledTextSize("80", 12.0f);
                    drawScaledText(ImVec2(healthBarX - textSize.x - 4.f, healthFilledY - textSize.y * 0.5f), ImColor(255, 255, 255), "80", 12.0f);
                }
            }
            if (cfg::esp::armor) {
                float armorBarX = charRight + 4.f;
                drawList->AddRectFilled(ImVec2(armorBarX - 1.f, charTop - 1.f), ImVec2(armorBarX + cfg::esp::bar_thickness + 1.f, charBottom + 1.f), ImColor(0, 0, 0, 150));
                drawList->AddRectFilled(ImVec2(armorBarX, charTop), ImVec2(armorBarX + cfg::esp::bar_thickness, charBottom), ImColor(0, 150, 255)); // 100% armor
            }
            
            // Flags mapping to right side
            float flagY = charTop;
            float flagX = charRight + (cfg::esp::armor ? (cfg::esp::bar_thickness + 10.f) : 4.f);
            auto drawFlag = [&](const char* text, ImColor color) {
                drawScaledText(ImVec2(flagX, flagY), color, text, 12.0f);
                flagY += 12.0f;
            };

            if (cfg::esp::flags::flashed) {
                drawFlag("FLASHED", cfgColor(cfg::esp::colors::flags::flashed_enemy));
            }
            if (cfg::esp::flags::reloading) {
                drawFlag("RELOAD", cfgColor(cfg::esp::colors::flags::reloading_enemy));
            }
            if (cfg::esp::flags::defusing) {
                drawFlag("DEFUSE", cfgColor(cfg::esp::colors::flags::defusing_enemy));
            }
            if (cfg::esp::flags::has_c4) {
                drawFlag("C4", cfgColor(cfg::esp::colors::flags::c4_enemy));
            }
            if (cfg::esp::flags::scoped) {
                drawFlag("SCOPED", cfgColor(cfg::esp::colors::flags::scoped_enemy));
            }
            
            // Top text
            float topStackY = charTop - 2.f;
            auto drawTopCenter = [&](const char* text, ImColor color) {
                ImVec2 textSize = calcScaledTextSize(text, 14.0f);
                topStackY -= textSize.y;
                drawScaledText(ImVec2(charLeft + (charWidth - textSize.x) * 0.5f, topStackY), color, text, 14.0f);
            };
            
            if (cfg::esp::flags::ping) {
                drawTopCenter("35ms", ImColor(200, 200, 200));
            }
            if (cfg::esp::flags::money) {
                drawTopCenter("$16000", ImColor(150, 255, 150));
            }
            if (cfg::esp::flags::name) {
                drawTopCenter("EnemyPlayer", ImColor(255, 255, 255));
            }

            // Bottom text
            float bottomStackY = charBottom + 2.f;
            auto drawBottomCenter = [&](const char* text, ImColor color) {
                ImVec2 textSize = calcScaledTextSize(text, 14.0f);
                drawScaledText(ImVec2(charLeft + (charWidth - textSize.x) * 0.5f, bottomStackY), color, text, 14.0f);
                bottomStackY += textSize.y;
            };

            if (cfg::esp::flags::weapon) {
                drawBottomCenter("AK-47", ImColor(255, 255, 255));
            }
            if (cfg::esp::flags::ammo) {
                drawBottomCenter("30/90", ImColor(200, 200, 200));
            }
        } else {
            ImGui::TextColored(notSelectedTextColor, "Preview image not loaded.");
        }
        
        ImGui::EndChild();

        ImGui::Columns(1);
    }
    // --------------------------------------------------------------- World
    else if (selectedSubTabVisuals == 1)
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 300.f);

        {
            // Bomb + Spectators
            ImGui::BeginChild("##world_left", ImVec2(ImGuiHelper::getWidth(), 200.f), true);

            ImGui::Text("Bomb");
            ImGui::Separator();
            ImGui::Checkbox("Bomb Location", &cfg::world::bomb::location);
            ImGui::Checkbox("Bomb Timer",    &cfg::world::bomb::timer);

            ImGui::Spacing();
            ImGui::Text("Spectator List");
            ImGui::Separator();
            ImGui::Checkbox("Enable##spec",  &cfg::world::spectators::enabled);
            if (cfg::world::spectators::enabled) {
                ImGui::Checkbox("Detailed",  &cfg::world::spectators::detailed);
                ImGui::Checkbox("Only Self", &cfg::world::spectators::self_only);
                ImGui::SetItemTooltip("Only display users spectating you");
            }

            ImGui::EndChild();
            ImGui::Spacing();

            // Misc + Radar
            ImGui::BeginChild("##world_misc", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), true);

            ImGui::Text("Misc");
            ImGui::Separator();
            ImGui::Checkbox("Crosshair",     &cfg::world::crosshair::enabled);
            ImGui::Checkbox("Velocity Graph",&cfg::world::velocity::enabled);
            ImGui::Checkbox("Ping",          &cfg::esp::flags::ping);

            ImGui::Spacing();
            ImGui::Text("Radar");
            ImGui::Separator();
            ImGui::Checkbox("Radar##radar",  &cfg::world::radar::enabled);
            ImGui::BeginDisabled(!cfg::world::radar::enabled);
            ImGui::SameLine();
            ImGui::SliderFloat("Range", &cfg::world::radar::range, 100.f, 8000.f, "%.0f u");
            ImGui::Checkbox("Disable Rotation", &cfg::world::radar::no_rotate);
            ImGui::EndDisabled();

            ImGui::EndChild();
        }

        ImGui::NextColumn();
        ImGui::BeginChild("##world_right", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), true);
        ImGui::TextColored(notSelectedTextColor, "More options soon...");
        ImGui::EndChild();

        ImGui::Columns(1);
    }
    // --------------------------------------------------------------- Other
    else
    {
        ImGui::BeginChild("##vis_other", ImVec2(ImGuiHelper::getWidth(), 100.f), true);
        ImGui::TextColored(notSelectedTextColor, "Nothing here yet...");
        ImGui::EndChild();
    }

    ImGui::EndChild(); // vis_wrap
}

// ============================================================================
//  Right panel — Misc
//  Sub-tabs: [General] [GUI]
// ============================================================================
void Menu::renderMiscTab() {
    const std::vector<std::string> subtabs = { "General", "GUI" };
    ImGuiHelper::drawTabHorizontally(
        "##misc_subtabs",
        ImVec2(ImGuiHelper::getWidth(), 50.f),
        subtabs,
        selectedSubTabMisc);
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild("##misc_wrap", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), false);
    ImGui::PopStyleColor();

    // --------------------------------------------------------- General
    if (selectedSubTabMisc == 0)
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 300.f);

        {
            ImGui::BeginChild("##misc_general", ImVec2(ImGuiHelper::getWidth(), 300.f), true);

            ImGui::Text("Application");
            ImGui::Separator();

            if (ImGui::Checkbox("Streamproof", &cfg::settings::streamproof)) {
                Window::SetAffinity(
                    Window::hwnd,
                    cfg::settings::streamproof ? WindowAffinity::Invisible : WindowAffinity::Disabled
                );
            }
            ImGui::Checkbox("Watermark", &cfg::settings::watermark);
            if (ImGui::Checkbox("VSync",       &cfg::settings::vsync))
                Window::vsync = cfg::settings::vsync;
            ImGui::Checkbox("Free CPU",  &cfg::settings::free_cpu);
            ImGui::SetItemTooltip(
                "Let the CPU sleep to free resources\n"
                "NOTE: may cause issues on lower-end machines.");

            ImGui::Spacing();
            ImGui::Text("Debug");
            ImGui::Separator();
            ImGui::Checkbox("Console", &cfg::dev::console);
            ImGui::SliderInt("Cache Refresh Rate", &cfg::dev::cache_refresh_rate, 0, 100, "%dms");
            ImGui::Checkbox("Force Show Flags",    &cfg::dev::force_show_flags);

            ImGui::EndChild();
            ImGui::Spacing();

            ImGui::BeginChild("##misc_notes", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), true);
            ImGui::Text("Performance Notes");
            ImGui::Separator();
            ImGui::TextWrapped(
                "If you experience lag try:\n"
                "  - Disable ESP VSync\n"
                "  - Disable VSync in-game\n"
                "  - Uncheck \"Free CPU\" as a last resort"
            );
            ImGui::EndChild();
        }

        ImGui::NextColumn();
        ImGui::BeginChild("##misc_right", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), true);
        ImGui::TextColored(notSelectedTextColor, "More options soon...");
        ImGui::EndChild();

        ImGui::Columns(1);
    }
    // ---------------------------------------------------------------- GUI
    else
    {
        ImGui::BeginChild("##misc_gui", ImVec2(ImGuiHelper::getWidth(), 280.f), true);

        ImGui::Text("GUI Colors");
        ImGui::Separator();

        static auto cf = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar;

        bool changed = false;
        changed |= ImGui::ColorEdit4("Window Color",        (float*)&winCol,             cf);
        changed |= ImGui::ColorEdit4("Background Color",    (float*)&childCol,           cf);
        changed |= ImGui::ColorEdit4("Frame Color",         (float*)&frameCol,           cf);
        changed |= ImGui::ColorEdit4("Button Color",        (float*)&bgCol,              cf);
        changed |= ImGui::ColorEdit4("Button Hovered",      (float*)&btnHoverCol,        cf);
        changed |= ImGui::ColorEdit4("Button Active",       (float*)&btnActiveCol,       cf);
        changed |= ImGui::ColorEdit4("Item Color",          (float*)&itemCol,            cf);
        changed |= ImGui::ColorEdit4("Item Active Color",   (float*)&itemActiveCol,      cf);

        if (changed)
            setColors(); // re-apply immediately

        ImGui::EndChild();
    }

    ImGui::EndChild(); // misc_wrap
}

// ============================================================================
//  Right panel — Config  (uses old project's Config::Read / Config::Write)
// ============================================================================
void Menu::renderConfigTab() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild("##cfg_wrap", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), false);
    ImGui::PopStyleColor();

    // -- Action bar
    ImGui::BeginChild("##cfg_bar", ImVec2(ImGuiHelper::getWidth(), 50.f), true,
                      ImGuiWindowFlags_NoScrollbar);

    if (bigFont) ImGui::PushFont(bigFont);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(10.f, 5.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);

    if (ImGui::Button(ICON_FA_SAVE " Save Config")) {
        if (Config::Write())
            LOGF(INFO, "Config saved successfully.");
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load Config")) {
        if (Config::Read())
            LOGF(INFO, "Config loaded successfully.");
    }

    ImGui::PopStyleVar(2);
    if (bigFont) ImGui::PopFont();

    ImGui::EndChild();
    ImGui::Spacing();

    // -- Info
    ImGui::BeginChild("##cfg_info", ImVec2(ImGuiHelper::getWidth(), ImGuiHelper::getHeight()), true);
    ImGui::Text("Config System");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped(
        "Save Config  - writes current settings to config.json\n"
        "Load Config  - reads saved settings from config.json\n\n"
        "Config is auto-saved when the menu is closed (End key)."
    );
    ImGui::EndChild();

    ImGui::EndChild(); // cfg_wrap
}

// ============================================================================
//  Main render
// ============================================================================
void Menu::RenderImpl() {
    if (!isSetup) return;

    auto& io     = ImGui::GetIO();
    auto  screen = io.DisplaySize;

    ImGui::SetNextWindowSize(ImVec2(800.f, 600.f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(screen.x * 0.5f - 400.f, screen.y * 0.5f - 300.f),
        ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    if (ImGui::Begin(" ##main_window", nullptr,
                     ImGuiWindowFlags_NoResize     |
                     ImGuiWindowFlags_NoCollapse   |
                     ImGuiWindowFlags_NoTitleBar   |
                     ImGuiWindowFlags_NoScrollbar  |
                     ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        this->pos  = ImGui::GetWindowPos();
        this->size = ImGui::GetWindowSize();

        // ------------------------------------------------------------------
        // 2-column split: left panel (173 px) | right content
        // ------------------------------------------------------------------
        ImGui::Columns(2, "##main_split", false);
        ImGui::SetColumnOffset(1, 173.f);

        // Left column
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 6.f));
        renderPanel();
        ImGui::PopStyleVar();

        // Right column
        ImGui::NextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));

        switch (selectedTab) {
        case LEGITBOT: renderLegitBotTab(); break;
        case VISUALS:  renderVisualsTab();  break;
        case MISC:     renderMiscTab();     break;
        case CONFIG:   renderConfigTab();   break;
        default:       break;
        }

        ImGui::PopStyleVar();
        ImGui::Columns(1);
    }
    ImGui::End();
    ImGui::PopStyleVar(); // WindowPadding

    // Render toast notifications (from imgui_notify.h)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,
        ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f));
    ImGui::RenderNotifications();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ============================================================================
//  Startup help overlay  (shown until menu is opened once)
// ============================================================================
void Menu::RenderStartupHelpImpl() {
    static bool has_opened_menu = false;
    if (has_opened_menu) return;

    auto& io    = ImGui::GetIO();
    auto  screen = io.DisplaySize;
    auto* d     = ImGui::GetBackgroundDrawList();

    if (Renderer::IsOpen())
        has_opened_menu = true;

    const char* help =
        "To OPEN the menu, press Insert or Right Shift\n"
        "                   To CLOSE, press End";
    auto sz = ImGui::CalcTextSize(help);

    d->AddText(
        ImVec2(screen.x * 0.5f - sz.x * 0.5f, 80.f),
        IM_COL32(255, 255, 255, 255),
        help);
}
#endif
