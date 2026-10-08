#include "ConfigPage.hpp"

#include <string>

#include "config/Config.hpp"
#include "gui/widgets/Widgets.hpp"

namespace menu_pages {

void RenderConfig(MenuContext&) {
    using namespace ui::widgets;
    ImGui::BeginChild("##config_page", {});

    SectionHeader("Configuration", "Versioned config.json with migration, validation, and atomic replacement.");
    if (ImGui::Button("Save now", { 120.0f, 0.0f }))
        Config::Write();
    ImGui::SameLine();
    if (ImGui::Button("Reload", { 120.0f, 0.0f }))
        Config::Read();

    ImGui::Spacing();
    StatusPill(Config::LastStatus().c_str(),
        Config::LastStatus().find("Failed") != std::string::npos || Config::LastStatus().find("invalid") != std::string::npos
            ? StatusKind::Danger : StatusKind::Neutral);

    SectionHeader("Reset", "Each reset requires an explicit confirmation and saves only after confirmation.");
    if (ConfirmButton("reset_esp", "Reset visuals", "Restore all player ESP settings and colors?"))
        Config::ResetEsp();
    ImGui::SameLine();
    if (ConfirmButton("reset_world", "Reset world", "Restore all world overlay positions and settings?"))
        Config::ResetWorld();
    ImGui::SameLine();
    if (ConfirmButton("reset_system", "Reset system", "Restore all application settings?"))
        Config::ResetSettings();

    ImGui::Spacing();
    if (ConfirmButton("reset_all", "Reset everything", "Restore every setting to defaults and overwrite config.json?"))
        Config::ResetAll();

    ImGui::EndChild();
}

} // namespace menu_pages
