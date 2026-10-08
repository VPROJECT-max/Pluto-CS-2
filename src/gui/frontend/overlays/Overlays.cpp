#include "Overlays.hpp"

#include "OverlayPresentation.hpp"

#include "gui/renderer/Renderer.hpp" // Circular dependency
#include "gui/frontend/menu/Menu.hpp" // Circular dependency
#include "assets/fonts/WeaponIcons.h"
#include "../../../../starline-imgui-menu-GOOD FOR CS 2/external_esp_menu.h"
#include <Windows.h>
#pragma comment(lib, "winmm.lib")

bool Overlays::Init() {
    return GetInstance().InitImpl();
}

void Overlays::Render() {
    return GetInstance().RenderImpl();
}

bool Overlays::InitImpl() {
    auto& io = ImGui::GetIO();

    ImFontConfig cfg{};
    cfg.FontDataOwnedByAtlas = false;

	this->font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 12.0f, &cfg);
	this->font_alt = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 14.0f, &cfg);
    this->font_icons = io.Fonts->AddFontFromMemoryTTF(weapon_icon_font, weapon_icon_font_len, 16.0f,  &cfg);
	
	ImFontConfig merge_icon_cfg{};
	merge_icon_cfg.FontDataOwnedByAtlas = false;
	merge_icon_cfg.MergeMode = true;

	static const ImWchar icon_ranges[] = { 0xE000, 0xE046, 0 };
	io.Fonts->AddFontFromMemoryTTF(weapon_icon_font, weapon_icon_font_len, 12.f, &merge_icon_cfg, icon_ranges);

    // Pre allocate buffer
    this->vel_buffer.resize(static_cast<size_t>(cfg::world::velocity::sample_rate * cfg::world::velocity::sample_length));

    return true;
}

void Overlays::RenderImpl() {
    ImGui::PushFont(this->font);
    {
        RenderWatermark();

        RenderNotice();
        RenderDefusingNotification();

        RenderDebugWindow();

    }
    ImGui::PopFont();

    ImGui::PushFont(this->font_alt);
    {
        RenderSpectatorList();
        RenderSpeedChart();
        RenderRadar();
        RenderBomb();
    }
    ImGui::PopFont();
}

void Overlays::RenderWatermark() {
    if (!cfg::settings::watermark)
        return;

    const SystemTelemetrySnapshot live = telemetry.Sample();
    Starline::GUI::RenderTokyoZKWatermark(
        ImGui::GetIO().Framerate,
        live.cpu_percent,
        live.working_set_mib);
}

void Overlays::RenderNotice() {
    // Release metadata is no longer used as an in-overlay notice channel.
}

inline Player* FindPlayerByPawnIndex(std::vector<Player>& players, int index) {
    Player* found = nullptr;

    for (auto& p : players) {
        if (p.pawn_controller_addr == index) {
            found = &p;
            break;
        }
    }
    return found;
}

void Overlays::RenderSpectatorList() {
    const auto visibility = overlay_presentation::ResolveVisibility(
        { .spectators = cfg::world::spectators::enabled }, {});
    if (!visibility.spectators)
        return;

    auto snapshot = Cache::CopySnapshot();
    auto& players = snapshot.players;

    const bool is_menu_open = Renderer::IsOpen();
    const bool detailed = cfg::world::spectators::detailed;
    const bool self_only = cfg::world::spectators::self_only;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    ImGuiTableFlags flags_table = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV;

    bool should_render = false;
    for (Player& p : players) {
        if (auto i = p.observer_services.target) {
            Player* target = FindPlayerByPawnIndex(players, i);

            if (self_only && (!target || !target->localplayer))
                continue;

            should_render = true;
            break;
        }
    }

    // Window
    ImGui::SetNextWindowPos(cfg::world::spectators::pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(150.f, 50.f), ImVec2(FLT_MAX, FLT_MAX));

    if (!ImGui::Begin("Spectator list", nullptr, flags)) {
        ImGui::End();
        return;
    }

    if (is_menu_open)
        cfg::world::spectators::pos = ImGui::GetWindowPos();

    if (!should_render) {
        ImGui::TextDisabled("No spectators");
        return ImGui::End();
    }

    if (detailed) {
        if (ImGui::BeginTable("##detailed", 3, flags_table)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Mode");
            ImGui::TableSetupColumn("Target");
            ImGui::TableHeadersRow();

            for (Player& player : players) {
                if (player.alive) continue;

                int targetIndex = player.observer_services.target;
                if (targetIndex == 0) continue;

                Player* target = FindPlayerByPawnIndex(players, targetIndex);

                if (self_only && (!target || !target->localplayer))
                    continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", player.name);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", player.observer_services.ToString());

                ImGui::TableSetColumnIndex(2);
                if (self_only) ImGui::Text("You");
                else if (player.observer_services.mode == ObserverMode::Free) ImGui::Text("No One");
                else ImGui::Text("%s", target ? target->name : "Invalid/bomb");
            }

            ImGui::EndTable();
        }
    }
    else {
        for (Player& player : players) {
            if (player.alive) continue;
            int targetIndex = player.observer_services.target;
            if (targetIndex == 0) continue;
            Player* target = FindPlayerByPawnIndex(players, targetIndex);

            if (self_only && (!target || !target->localplayer)) continue;

            ImGui::Text("%s", player.name);
        }
    }

    ImGui::End();
}

void Overlays::RenderSpeedChart() {
    const auto visibility = overlay_presentation::ResolveVisibility(
        { .velocity = cfg::world::velocity::enabled }, {});
    if (!visibility.velocity)
        return;

    auto& io = ImGui::GetIO();
    auto d = ImGui::GetBackgroundDrawList();

    auto snapshot = Cache::CopySnapshot();
    auto& local = snapshot.local;

    const static float padding = 10.0f;
    const bool is_menu_open = Renderer::IsOpen();

    auto& pos = cfg::world::velocity::pos;
    auto& size = cfg::world::velocity::size;

    int rate = cfg::world::velocity::sample_rate;
    float length = cfg::world::velocity::sample_length;

    static int prev_rate = rate;
    static float prev_length = length;

    float left = pos.x + padding;
    float right = pos.x + size.x - padding;
    float bottom = pos.y + size.y - padding;
    float top = pos.y + padding;

    float width = right - left;
    float height = bottom - top;

    if (is_menu_open) {
        auto height_padding = 25; // some padding to keep the speed number inside the area
        auto altitude_padding = 10; // so it doesnt go under the titlebar

        ImGui::SetNextWindowBgAlpha(0.1f);
        ImGui::SetNextWindowPos(pos - Vec2_t(0, altitude_padding), ImGuiCond_Once);
        ImGui::SetNextWindowSize(size + Vec2_t(0, height_padding), ImGuiCond_Once);
        if (ImGui::Begin("Velocity Graph", nullptr, ImGuiWindowFlags_NoCollapse))
        {
            pos = ImGui::GetWindowPos() + ImVec2(0, altitude_padding);
            size = ImGui::GetWindowSize() - ImVec2(0, height_padding);
            ImGui::End();
        }
    }

    // Cache menu values and resize when changed
    if (prev_rate != rate || prev_length != length) {
        prev_rate = rate;
        prev_length = length;

        vel_buffer.resize(static_cast<size_t>(rate * length));
    }

    const Vec2_t speed_2d(local.vel.x, local.vel.y);
    const int speed = local.alive ? static_cast<int>(std::floor(speed_2d.len())) : 0;

    vel_accumulator += io.DeltaTime;
    size_t buff_size = vel_buffer.size();

    std::vector<ImVec2> points;
    points.reserve(buff_size);

    float sample_interval = 1.0f / rate;

    while (vel_accumulator >= sample_interval)
    {
        vel_accumulator -= sample_interval;
        vel_buffer.at(vel_index % buff_size) = speed;
        vel_index = (vel_index + 1) % buff_size;
    }

    int max_speed = 1;
    for (int v : vel_buffer)
        max_speed = std::max(max_speed, v);

    for (size_t i = 0; i < buff_size; ++i) {
        float t = i / float(buff_size - 1);

        float x = left + t * width;

        float normalized =
            vel_buffer[(i + vel_index) % buff_size] / float(max_speed);

        float y = bottom - (normalized * height);

        points.emplace_back(x, y);
    }

    d->AddPolyline(
        points.data(),
        static_cast<int>(points.size()),
        IM_COL32(255, 255, 255, 255),
        ImDrawFlags_None,
        1.0f
    );

    auto center = ImVec2(
        pos.x + size.x / 2,
        pos.y + size.y
    );

    d->AddText(
        center,
        IM_COL32(255, 255, 255, 255),
        std::to_string(speed).c_str());
}

void Overlays::RenderDebugWindow() {
    auto& io = ImGui::GetIO();
    auto d = ImGui::GetBackgroundDrawList();

    auto snapshot = Cache::CopySnapshot();
    auto& game = snapshot.game;
    auto& bomb = snapshot.bomb;
    auto& globals = snapshot.globals;
    auto& players = snapshot.players;

    static int margin = 10;
    static int padding = 10;
    std::string debug_string = "> Game Debug Window\n";

    debug_string += std::format("Map: {}\n", globals.map_name);
    debug_string += std::format("Max Clients: {}\n", globals.max_clients);
    debug_string += std::format("Cache Refresh: {}ms\n", cfg::dev::cache_refresh_rate);

    if (bomb.is_planted) {
        debug_string += "Bomb:\n";
        debug_string += std::format("- Planted Site: {}\n", bomb.site == BombSite::A ? "A" : "B");
    }

    if (!players.empty())
        debug_string += std::format("Players ({}):\n", players.size());

	for (auto& player : players)
		debug_string += std::format(
			"- [{}] {} {}hp {} {}\n", 
			player.index, player.name, 
			player.health, player.weapon.name,
			player.weapon.icon
		);

    auto size = ImGui::CalcTextSize(debug_string.data());

    d->AddRectFilled(
        ImVec2(10 + margin - padding, io.DisplaySize.y - size.y - 20 - margin - padding),
        ImVec2(10 + size.x + margin + padding, io.DisplaySize.y - 20 - margin + padding),
        IM_COL32(0, 0, 0, 200),
        10.f
    );

    d->AddRect(
        ImVec2(10 + margin - padding, io.DisplaySize.y - size.y - 20 - margin - padding),
        ImVec2(10 + size.x + margin + padding, io.DisplaySize.y - 20 - margin + padding),
        IM_COL32(100, 100, 100, 200),
        10.f
    );

    d->AddText(
        ImVec2(10 + margin, io.DisplaySize.y - size.y - 20 - margin),
        IM_COL32(255, 255, 255, 255),
        debug_string.data()
    );
}

void Overlays::RenderRadar() {
    const auto visibility = overlay_presentation::ResolveVisibility(
        { .radar = cfg::world::radar::enabled }, {});
    if (!visibility.radar)
        return;

    auto snapshot = Cache::CopySnapshot();
    auto& local = snapshot.local;
    auto& players = snapshot.players;
    auto& matrix = snapshot.game.view_matrix;

    const bool is_menu_open = Renderer::IsOpen();

    auto& pos = cfg::world::radar::pos;
    auto& size = cfg::world::radar::size;
    float range = cfg::world::radar::range;

    if (is_menu_open) {
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(size, ImGuiCond_Once);
        if (ImGui::Begin("Radar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            pos = ImGui::GetWindowPos();
            size = ImGui::GetWindowSize();
            ImGui::End();
        }
    }

    auto d = ImGui::GetBackgroundDrawList();

    const float cx = pos.x + size.x * 0.5f;
    const float cy = pos.y + size.y * 0.5f;
    const float rx = size.x * 0.5f;
    const float ry = size.y * 0.5f;
    const float radius = std::min(rx, ry);

    d->AddRectFilled(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(0, 0, 0, 50),
        6.f
    );

    d->AddRect(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(80, 80, 80, 100),
        6.f
    );

    d->AddCircle(ImVec2(cx, cy), radius * 0.333f, IM_COL32(50, 50, 50, 120));
    d->AddCircle(ImVec2(cx, cy), radius * 0.666f, IM_COL32(50, 50, 50, 120));
    d->AddLine(ImVec2(pos.x + 4.f, cy), ImVec2(pos.x + size.x - 4.f, cy), IM_COL32(50, 50, 50, 120));
    d->AddLine(ImVec2(cx, pos.y + 4.f), ImVec2(cx, pos.y + size.y - 4.f), IM_COL32(50, 50, 50, 120));

    for (auto& player : players) {
        if (!local.alive)
            break;
        if (!player.alive)
            continue;

        if (player.localplayer)
            continue;

        Vec3_t delta = player.pos - local.pos;
        float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);

        if (dist > range)
            continue;

        float nx = delta.x / range;
        float ny = delta.y / range;

        float sx, sy;
        if (!cfg::world::radar::no_rotate) {
            float rx = matrix[0][0];
            float ry = matrix[0][1];
            float len = sqrtf(rx * rx + ry * ry);
            if (len > 0.001f) { rx /= len; ry /= len; }
            float fx = -ry;
            float fy =  rx;
            float rad_x = nx * rx + ny * ry;
            float rad_y = nx * fx + ny * fy;
            sx = cx + rad_x * (size.x * 0.5f - 6.f);
            sy = cy - rad_y * (size.y * 0.5f - 6.f);
        } else {
            sx = cx + nx * (size.x * 0.5f - 6.f);
            sy = cy - ny * (size.y * 0.5f - 6.f);
        }

        bool mate = player.team == local.team;
        ImU32 col = mate
            ? IM_COL32(0, 220, 80, 255)
            : IM_COL32(220, 50, 50, 255);

        d->AddCircleFilled(ImVec2(sx, sy), 4.f, col);
        d->AddCircle(ImVec2(sx, sy), 4.f, IM_COL32(0, 0, 0, 180));
    }

    d->AddCircleFilled(ImVec2(cx, cy), 5.f, IM_COL32(100, 180, 255, 255));
    d->AddCircle(ImVec2(cx, cy), 5.f, IM_COL32(0, 0, 0, 180));

    d->AddText(ImVec2(pos.x + 6.f, pos.y + 4.f), IM_COL32(180, 180, 180, 200), "Radar");
}

void Overlays::RenderBomb() {
    const auto visibility = overlay_presentation::ResolveVisibility(
        {
            .bomb_location = cfg::world::bomb::location,
            .bomb_timer = cfg::world::bomb::timer,
        },
        {});
    if (!visibility.bomb)
        return;

    auto& io = ImGui::GetIO();
    auto snapshot = Cache::CopySnapshot();

    auto& bomb = snapshot.bomb;
    auto& local = snapshot.local;
    auto& matrix = snapshot.game.view_matrix;

    const bool is_menu_open = Renderer::IsOpen();

    float width = 20.f;
    float height = 20.f;
    float rounding = 4.f;

    static int margin = 4;
    static int padding = 6;
    float bar_height = 3.f;
    float element_gap = 6.f;

    const auto presentation = overlay_presentation::BuildBombPresentation(
        cfg::world::bomb::location,
        cfg::world::bomb::timer,
        bomb.is_planted,
        bomb.site == BombSite::B ? 'B' : 'A',
        bomb.time_left);
    const std::string& bomb_string = presentation.text;

    auto text_size = ImGui::CalcTextSize(bomb_string.data());

    ImGui::PushFont(this->font_icons);
    auto icon_size = ImGui::CalcTextSize(WeaponIcons::C4);
    ImGui::PopFont();

    float content_width = icon_size.x + element_gap + text_size.x;
    float content_height = std::max(icon_size.y, text_size.y);

    width = content_width + (padding * 2);
    height = content_height + (padding * 2) + (presentation.show_progress ? bar_height + 2.f : 0.f);

    if (is_menu_open) {
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        float title_bar_height = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(cfg::world::bomb::pos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(width, height + title_bar_height), ImGuiCond_Always);

        if (ImGui::Begin("Bomb Window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {
            cfg::world::bomb::pos = ImGui::GetWindowPos();
            ImGui::End();
        }

        ImGui::PopStyleVar();
    }

    Vec2_t screen_pos;
    const bool has_world_location = bomb.is_planted && local.alive && bomb.pos.length() > 0.0f;
    bool on_top = has_world_location && matrix.wts(bomb.pos, io.DisplaySize, screen_pos);

    const float dist = has_world_location ? local.pos.dist_to(bomb.pos) : 0.0f;

    if (is_menu_open) on_top = false;
    if (dist > 1500.f) on_top = false;

    float render_x = 0.f;
    float render_y = 0.f;

    if (on_top && bomb.is_planted) {
        render_x = screen_pos.x - (width * 0.5f);
        render_y = screen_pos.y + margin;
    }
    else {
        float title_bar_height = ImGui::GetFrameHeight();
        render_x = cfg::world::bomb::pos.x;
        render_y = cfg::world::bomb::pos.y + title_bar_height;
    }

    auto d = ImGui::GetBackgroundDrawList();

    d->AddRectFilled(
        ImVec2(render_x, render_y),
        ImVec2(render_x + width, render_y + height),
        IM_COL32(15, 15, 15, 220),
        rounding
    );

    d->AddRect(
        ImVec2(render_x, render_y),
        ImVec2(render_x + width, render_y + height),
        IM_COL32(45, 45, 45, 255),
        rounding
    );

    d->AddText(
        this->font_icons,
        16.0f,
        ImVec2(render_x + padding, render_y + padding + (content_height - icon_size.y) * 0.5f),
        IM_COL32(255, 60, 60, 255),
        WeaponIcons::C4
    );

    d->AddText(
        ImVec2(render_x + padding + icon_size.x + element_gap, render_y + padding + (content_height - text_size.y) * 0.5f),
        IM_COL32(240, 240, 240, 255),
        bomb_string.data()
    );

    if (presentation.show_progress)
    {
        const float progress = presentation.progress;

        ImU32 bar_color = progress > 0.5f
            ? IM_COL32((int)((1.f - progress) * 2.f * 255), 220, 50, 255)
            : IM_COL32(220, (int)(progress * 2.f * 220), 50, 255);

        ImVec2 bar_start(render_x + rounding, render_y + height - bar_height - 4.f);
        ImVec2 bar_end(render_x + width - rounding, render_y + height - bar_height - 4.f);
        float max_bar_width = bar_end.x - bar_start.x;

        d->AddLine(bar_start, bar_end, IM_COL32(40, 40, 40, 255), bar_height);

        if (progress > 0.f)
        {
            ImVec2 bar_filled_end(bar_start.x + (max_bar_width * progress), bar_end.y);
            d->AddLine(bar_start, bar_filled_end, bar_color, bar_height);
        }
    }
}

void Overlays::RenderDefusingNotification() {
    if (!cfg::settings::defusal_notification)
        return;

    static float start_time = 0.0f;
    static bool was_defusing = false;
    
    auto snapshot = Cache::CopySnapshot();
    auto& bomb = snapshot.bomb;

    // Detect state change from not-defusing to defusing
    if (bomb.is_planted && bomb.is_being_defused && !was_defusing) {
        start_time = ImGui::GetTime();
        // Play notification sound with mciSendString to control volume
        // Since mciSendString can handle mp3 and volume easily
        mciSendStringA("close defuse_snd", nullptr, 0, nullptr);
        mciSendStringA("open \"C:\\Users\\ddeni\\Downloads\\myinstants.mp3\" type mpegvideo alias defuse_snd", nullptr, 0, nullptr);
        mciSendStringA("setaudio defuse_snd volume to 300", nullptr, 0, nullptr); // Lower volume (0-1000)
        mciSendStringA("play defuse_snd", nullptr, 0, nullptr);
    }
    
    was_defusing = bomb.is_being_defused;

    float current_time = ImGui::GetTime();
    float elapsed = current_time - start_time;
    float duration = 3.0f; // 3 seconds duration
    
    if (elapsed > duration)
        return;

    auto& io = ImGui::GetIO();
    auto d = ImGui::GetBackgroundDrawList();

    // Animation calculation
    float progress = elapsed / duration;
    
    // Fade out at the end, but also fade in at the start
    float alpha = 1.0f;
    if (progress < 0.1f) alpha = progress / 0.1f;
    else if (progress > 0.8f) alpha = 1.0f - ((progress - 0.8f) / 0.2f);
    
    // Move up animation
    float y_offset = 60.0f - (progress * 40.0f); // Starts at Y=60, moves up to Y=20

    std::string text = "Bomb is being Defused!";
    ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    
    float padding_x = 15.0f;
    float padding_y = 10.0f;
    
    ImVec2 rect_start(io.DisplaySize.x * 0.5f - text_size.x * 0.5f - padding_x, y_offset);
    ImVec2 rect_end(io.DisplaySize.x * 0.5f + text_size.x * 0.5f + padding_x, y_offset + text_size.y + padding_y * 2.0f);
    
    // Background
    d->AddRectFilled(
        rect_start, 
        rect_end, 
        IM_COL32(20, 20, 20, (int)(220 * alpha)), 
        8.0f
    );
    
    // Border
    d->AddRect(
        rect_start, 
        rect_end, 
        IM_COL32(200, 40, 40, (int)(255 * alpha)), 
        8.0f,
        0,
        2.0f
    );

    // Text
    d->AddText(
        ImVec2(rect_start.x + padding_x, rect_start.y + padding_y),
        IM_COL32(255, 255, 255, (int)(255 * alpha)),
        text.c_str()
    );
}
