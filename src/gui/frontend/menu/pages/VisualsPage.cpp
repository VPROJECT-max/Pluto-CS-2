#include "VisualsPage.hpp"

#include <array>

#include "config/Current.hpp"
#include "gui/frontend/preview/EspPreview.hpp"
#include "gui/widgets/Widgets.hpp"

namespace menu_pages {

void RenderVisuals(MenuContext&) {
    using namespace ui::widgets;
    if (!ImGui::BeginTable("##visuals_layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        return;

    ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 1.15f);
    ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.85f);
    ImGui::TableNextColumn();
    ImGui::BeginChild("##visual_controls", {}, false);

    SectionHeader("Player ESP", "Clean, distance-aware drawing with stable screen-space layout.");
    Toggle("esp_enabled", "Master enable", &cfg::enabled);
    Toggle("esp_team", "Show team", &cfg::esp::team);
    Toggle("esp_spotted", "Spotted only", &cfg::esp::spotted);
    Toggle("esp_visible", "Visible-state colors", &cfg::esp::visible_check);
    Toggle("esp_box", "Box", &cfg::esp::box);
    static constexpr std::array<const char*, 2> box_styles{ "Full", "Corners" };
    Select("box_style", "Box style", &cfg::esp::box_style, box_styles);
    SliderFloat("box_thickness", "Box thickness", &cfg::esp::box_thickness, 1.0f, 6.0f, "%.1f px");
    Toggle("outline", "Outline", &cfg::esp::outline);
    Toggle("skeleton", "Skeleton", &cfg::esp::skeleton);
    SliderFloat("skeleton_thickness", "Skeleton thickness", &cfg::esp::skeleton_thickness, 1.0f, 6.0f, "%.1f px");
    Toggle("head_tracker", "Head marker", &cfg::esp::head_tracker);
    Toggle("eye_ray", "Eye direction", &cfg::esp::eye_ray);
    Toggle("tracers", "Tracers", &cfg::esp::tracers);
    Toggle("chams", "Silhouette overlay", &cfg::esp::chams);

    SectionHeader("Bars and labels");
    Toggle("health", "Health bar", &cfg::esp::health);
    Toggle("health_number", "Health number", &cfg::esp::health_number);
    Toggle("armor", "Armor bar", &cfg::esp::armor);
    SliderFloat("bar_thickness", "Bar thickness", &cfg::esp::bar_thickness, 1.0f, 6.0f, "%.1f px");
    SliderFloat("text_scale", "Text scale", &cfg::esp::text_scale, 0.75f, 1.5f, "%.2fx");
    Toggle("flag_name", "Name", &cfg::esp::flags::name);
    Toggle("flag_weapon", "Weapon", &cfg::esp::flags::weapon);
    Toggle("flag_ammo", "Ammo", &cfg::esp::flags::ammo);
    Toggle("flag_distance", "Distance", &cfg::esp::flags::distance);
    Toggle("flag_ping", "Ping", &cfg::esp::flags::ping);
    Toggle("flag_money", "Money", &cfg::esp::flags::money);
    Toggle("flag_flashed", "Flashed", &cfg::esp::flags::flashed);
    Toggle("flag_reloading", "Reloading", &cfg::esp::flags::reloading);
    Toggle("flag_defusing", "Defusing", &cfg::esp::flags::defusing);
    Toggle("flag_scoped", "Scoped", &cfg::esp::flags::scoped);
    Toggle("flag_c4", "C4 carrier", &cfg::esp::flags::has_c4);

    SectionHeader("Distance behavior");
    Toggle("offscreen_indicators", "Off-screen indicators", &cfg::esp::offscreen_indicators);
    SliderFloat("fade_start", "Fade starts", &cfg::esp::fade_start, 0.0f, 500.0f, "%.0f m");
    SliderFloat("fade_end", "Fade ends", &cfg::esp::fade_end, 1.0f, 500.0f, "%.0f m");
    SliderFloat("max_distance", "Maximum distance", &cfg::esp::max_distance, 0.0f, 500.0f, "%.0f m");

    SectionHeader("Primary colors");
    Color("box_enemy", "Enemy box", &cfg::esp::colors::box_enemy);
    Color("box_enemy_visible", "Visible enemy", &cfg::esp::colors::box_enemy_visible);
    Color("box_team", "Team box", &cfg::esp::colors::box_team);
    Color("skeleton_enemy", "Enemy skeleton", &cfg::esp::colors::skeleton_enemy);
    ImGui::EndChild();

    ImGui::TableNextColumn();
    ImGui::BeginChild("##preview_panel", {}, false);
    SectionHeader("Live preview", "Deterministic mock data; no game process reads.");
    EspPreview::Render(ImGui::GetContentRegionAvail());
    ImGui::EndChild();
    ImGui::EndTable();
}

} // namespace menu_pages
