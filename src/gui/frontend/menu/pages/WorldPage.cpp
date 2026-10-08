#include "WorldPage.hpp"

#include "config/Current.hpp"
#include "gui/widgets/Widgets.hpp"

namespace menu_pages {

void RenderWorld(MenuContext&) {
    using namespace ui::widgets;
    ImGui::BeginChild("##world_page", {});

    SectionHeader("Spectators", "Compact observer information without visual noise.");
    Toggle("spectators_enabled", "Enable spectator list", &cfg::world::spectators::enabled);
    Toggle("spectators_detailed", "Detailed rows", &cfg::world::spectators::detailed);
    Toggle("spectators_self", "Local player only", &cfg::world::spectators::self_only);
    SliderFloat("spectators_x", "Horizontal position", &cfg::world::spectators::pos.x, 0.0f, 3840.0f, "%.0f px");
    SliderFloat("spectators_y", "Vertical position", &cfg::world::spectators::pos.y, 0.0f, 2160.0f, "%.0f px");

    SectionHeader("Bomb");
    Toggle("bomb_location", "Show location", &cfg::world::bomb::location);
    Toggle("bomb_timer", "Show timer", &cfg::world::bomb::timer);
    SliderFloat("bomb_x", "Horizontal position", &cfg::world::bomb::pos.x, 0.0f, 3840.0f, "%.0f px");
    SliderFloat("bomb_y", "Vertical position", &cfg::world::bomb::pos.y, 0.0f, 2160.0f, "%.0f px");

    SectionHeader("Radar and movement");
    Toggle("crosshair", "Sniper crosshair", &cfg::world::crosshair::enabled);
    Toggle("radar_enabled", "Radar", &cfg::world::radar::enabled);
    Toggle("radar_rotation", "Disable rotation", &cfg::world::radar::no_rotate);
    SliderFloat("radar_range", "Radar range", &cfg::world::radar::range, 100.0f, 8000.0f, "%.0f u");
    SliderFloat("radar_x", "Radar X", &cfg::world::radar::pos.x, 0.0f, 3840.0f, "%.0f px");
    SliderFloat("radar_y", "Radar Y", &cfg::world::radar::pos.y, 0.0f, 2160.0f, "%.0f px");
    SliderFloat("radar_width", "Radar width", &cfg::world::radar::size.x, 120.0f, 600.0f, "%.0f px");
    SliderFloat("radar_height", "Radar height", &cfg::world::radar::size.y, 120.0f, 600.0f, "%.0f px");
    Toggle("velocity_enabled", "Velocity graph", &cfg::world::velocity::enabled);
    SliderInt("velocity_rate", "Sample rate", &cfg::world::velocity::sample_rate, 1, 100, "%d ms");
    SliderFloat("velocity_length", "History length", &cfg::world::velocity::sample_length, 1.0f, 15.0f, "%.1f s");
    SliderFloat("velocity_x", "Graph X", &cfg::world::velocity::pos.x, 0.0f, 3840.0f, "%.0f px");
    SliderFloat("velocity_y", "Graph Y", &cfg::world::velocity::pos.y, 0.0f, 2160.0f, "%.0f px");
    SliderFloat("velocity_width", "Graph width", &cfg::world::velocity::size.x, 200.0f, 900.0f, "%.0f px");
    SliderFloat("velocity_height", "Graph height", &cfg::world::velocity::size.y, 80.0f, 400.0f, "%.0f px");

    ImGui::EndChild();
}

} // namespace menu_pages
