#include "SystemPage.hpp"

#include "config/Current.hpp"
#include "gui/renderer/window/Window.hpp"
#include "gui/widgets/Widgets.hpp"

namespace menu_pages {

void RenderSystem(MenuContext& context) {
    using namespace ui::widgets;
    ImGui::BeginChild("##system_page", {});

    SectionHeader("Application", "Behavior that affects the overlay as a whole.");
    if (Toggle("streamproof", "Hide from capture", &cfg::settings::streamproof,
            "Uses the Windows display-affinity API.")) {
        Window::SetAffinity(Window::hwnd,
            cfg::settings::streamproof ? WindowAffinity::Invisible : WindowAffinity::Disabled);
    }
    Toggle("watermark", "Watermark", &cfg::settings::watermark);
    if (Toggle("vsync", "VSync", &cfg::settings::vsync))
        Window::SetVSync(cfg::settings::vsync);
    Toggle("free_cpu", "Reduce CPU use", &cfg::settings::free_cpu,
        "Yields CPU time between frames. Disable only if frame pacing is uneven.");
    Toggle("third_person", "Force third person", &cfg::settings::force_third_person);
    Toggle("defusal_notification", "Defusal notification", &cfg::settings::defusal_notification);
    Toggle("debug_overlay", "Debug overlay", &cfg::settings::debug_overlay,
        "Shows live map, cache, bomb, and player diagnostics.");

    SectionHeader("Interface");
    Toggle("diagnostics", "Show diagnostics", &context.show_diagnostics);
    ImGui::TextDisabled("DPI scale: %.2fx", context.dpi_scale);

    SectionHeader("Developer");
    Toggle("console", "Console", &cfg::dev::console);
    SliderInt("cache_rate", "Cache refresh", &cfg::dev::cache_refresh_rate, 1, 100, "%d ms");
    Toggle("force_flags", "Force player flags", &cfg::dev::force_show_flags);

    ImGui::EndChild();
}

} // namespace menu_pages
