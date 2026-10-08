#include "Esp.hpp"

#include "gui/renderer/Renderer.hpp"
#include "gui/renderer/window/Window.hpp"
#include "assets/fonts/WeaponIcons.h"
#include "assets/fonts/Icons.h"

namespace {

ImU32 FadeColor(const color_t& color, float alpha) {
    return ImColor(color.r, color.g, color.b, color.a * std::clamp(alpha, 0.0f, 1.0f));
}

ImVec2 EdgeProjection(const view_matrix_t& matrix, const Player& source, const Player& player, ImVec2 display) {
    const Vec3_t direction = player.pos - source.pos;
    float x = matrix.matrix[0][0] * direction.x + matrix.matrix[0][1] * direction.y + matrix.matrix[0][2] * direction.z;
    float y = matrix.matrix[1][0] * direction.x + matrix.matrix[1][1] * direction.y + matrix.matrix[1][2] * direction.z;
    const float z = matrix.matrix[2][0] * direction.x + matrix.matrix[2][1] * direction.y + matrix.matrix[2][2] * direction.z;
    if (z > 0.0f) {
        x = -x;
        y = -y;
    }

    const ImVec2 normalized = esp_draw::NormalizeDirection({ x, -y });
    return {
        display.x * 0.5f + normalized.x * display.x,
        display.y * 0.5f + normalized.y * display.y,
    };
}

} // namespace

void SetChamsRenderTarget(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	if (!Window::device_context || !Window::chams_rtv)
		return;
    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    Window::device_context->OMSetRenderTargets(1, &Window::chams_rtv, nullptr);
    Window::device_context->ClearRenderTargetView(Window::chams_rtv, clear_color);
}

void RestoreRenderTarget(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	if (!Window::device_context || !Window::render_targetview)
		return;
    Window::device_context->OMSetRenderTargets(1, &Window::render_targetview, nullptr);
}

bool Esp::Init() {
	return GetInstance().InitImpl();
}

void Esp::Render() {
    return GetInstance().RenderImpl();
}

bool Esp::InitImpl() {
	auto& io = ImGui::GetIO();

	ImFontConfig cfg{};
	cfg.FontDataOwnedByAtlas = false;

	this->font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 12.0f, &cfg);

	this->font_merged_icons = io.Fonts->AddFontFromMemoryTTF(
		weapon_icon_font,
		weapon_icon_font_len,
		16.0f,
		&cfg
	);

	cfg.MergeMode = true;

	static const ImWchar general_ranges[] = { 0xE100, 0xE108, 0 };
	io.Fonts->AddFontFromMemoryTTF(
		icons_font,
		icons_font_len,
		16.0f,
		&cfg,
		general_ranges
	);

	return true;
}

void Esp::RenderImpl() {
	if (!cfg::enabled)
		return;

	auto snapshot = Cache::CopySnapshot();
	auto& game = snapshot.game;
	auto& bomb = snapshot.bomb;
	auto& local = snapshot.local;
	auto& globals = snapshot.globals;
	auto& players = snapshot.players;
	
	ImGui::PushFont(this->font);

	this->io = ImGui::GetIO();
	this->d = ImGui::GetBackgroundDrawList();

	this->matrix = game.view_matrix;
	this->local = local;

	const bool chams_ready = cfg::esp::chams && Window::CreateChamsRenderTarget(
		static_cast<UINT>(std::max(io.DisplaySize.x, 0.0f)),
		static_cast<UINT>(std::max(io.DisplaySize.y, 0.0f))
	);

	if (chams_ready) {
		d->AddCallback(SetChamsRenderTarget, nullptr);
		
		for (auto& player : players) {
			if (!player.alive || player.localplayer) continue;
			bool mate = player.team == local.team;
			if (!cfg::esp::team && mate) continue;
			if (cfg::esp::spotted && !player.spotted) continue;
			if (local.observer_services.target == player.pawn_controller_addr && local.observer_services.mode == ObserverMode::First) continue;
			const float distance_m = local.pos.dist_to_3d(player.pos) * 0.0254f;
			if (cfg::esp::max_distance > 0.0f && distance_m > cfg::esp::max_distance) continue;
			
			RenderPlayerChams(player, mate);
		}

		d->AddCallback(RestoreRenderTarget, nullptr);
		
		float global_alpha = cfg::esp::colors::chams_enemy.a;
		d->AddImage((ImTextureID)Window::chams_srv, ImVec2(0, 0), io.DisplaySize, ImVec2(0, 0), ImVec2(1, 1), ImColor(1.0f, 1.0f, 1.0f, global_alpha));
	}

	for (auto& player : players) {
		if (!player.alive)
			continue;

		if (player.localplayer)
			continue;

		bool mate = player.team == local.team;

		if (!cfg::esp::team && mate)
			continue;

		if (cfg::esp::spotted && !player.spotted)
			continue;

		// Are we spectating the player in first person? then dont render
		// TODO: Exception here when spectating someone
		if (
			local.observer_services.target == player.pawn_controller_addr
			&& local.observer_services.mode == ObserverMode::First
		)
			continue;

		PlayerLayout layout{};
		if (!BuildPlayerLayout(player, mate, layout))
			continue;

		if (!layout.on_screen) {
			if (cfg::esp::offscreen_indicators)
				esp_draw::AddOffscreenIndicator(*d, io.DisplaySize, layout.projected, layout.distance_m,
					FadeColor(mate ? cfg::esp::colors::box_team : cfg::esp::colors::box_enemy, layout.alpha), 26.0f);
			continue;
		}

		RenderPlayerTracers(local, player, layout);
		RenderPlayer(player, layout);
	}

	RenderCrosshair(local);
	ImGui::PopFont();
}

bool Esp::BuildPlayerLayout(Player player, bool mate, PlayerLayout& layout) {
	layout.mate = mate;
	layout.visible = cfg::esp::visible_check && player.spotted;
	layout.distance_m = this->local.pos.dist_to_3d(player.pos) * 0.0254f;
	if (cfg::esp::max_distance > 0.0f && layout.distance_m > cfg::esp::max_distance)
		return false;

	layout.alpha = esp_draw::DistanceAlpha(layout.distance_m, cfg::esp::fade_start, cfg::esp::fade_end);
	if (layout.alpha <= 0.0f)
		return false;

	Vec2_t projected{};
	layout.on_screen = matrix.wts(player.pos, io.DisplaySize, projected, true);
	if (layout.on_screen)
		layout.projected = projected;
	else
		layout.projected = EdgeProjection(matrix, this->local, player, io.DisplaySize);

	std::pair<Vec2_t, Vec2_t> bounds;
	if (!player.GetBounds(matrix, io.DisplaySize, bounds)) {
		layout.on_screen = false;
		return true;
	}

	layout.bounds = { bounds.first, bounds.second };
	layout.on_screen = layout.bounds.Valid();
	return true;
}

void Esp::RenderPlayer(Player player, const PlayerLayout& layout) {

	// Causes hp bars across the screen when they respawn
	if (!player.alive)
		return;

	if (cfg::esp::box) {
		const auto color = layout.mate ?
			(layout.visible ? cfg::esp::colors::box_team_visible : cfg::esp::colors::box_team) :
			(layout.visible ? cfg::esp::colors::box_enemy_visible : cfg::esp::colors::box_enemy);
		if (cfg::esp::box_style == 1)
			esp_draw::AddCornerBox(*d, layout.bounds, FadeColor(color, layout.alpha), cfg::esp::box_thickness, 0.25f, cfg::esp::outline);
		else
			esp_draw::AddBox(*d, layout.bounds, FadeColor(color, layout.alpha), cfg::esp::box_thickness, cfg::esp::outline);
	}

	if (cfg::esp::skeleton)
		RenderPlayerBones(player, layout);

	if (cfg::esp::head_tracker)
		RenderPlayerTracker(player, layout);

    if (cfg::esp::eye_ray)
        RenderPlayerEyeRay(player, layout);

	RenderPlayerBars(player, layout);
	RenderPlayerFalgs(player, layout);
}

void Esp::RenderPlayerBones(Player player, const PlayerLayout& layout) {
	auto color = layout.mate ?
        (layout.visible ? cfg::esp::colors::skeleton_team_visible : cfg::esp::colors::skeleton_team) :
        (layout.visible ? cfg::esp::colors::skeleton_enemy_visible : cfg::esp::colors::skeleton_enemy);

	auto bone_count = player.bone_list.size();
	for (const auto& bone : connections) {
		int first = bone[0], second = bone[1];

		if (bone_count <= first || bone_count <= second)
			continue;

		const auto& bone1 = player.bone_list[first];
		const auto& bone2 = player.bone_list[second];

		Vec2_t scb1;
		if (!matrix.wts(bone1.pos, io.DisplaySize, scb1))
			continue;

		Vec2_t scb2;
		if (!matrix.wts(bone2.pos, io.DisplaySize, scb2))
			continue;

		if (cfg::esp::outline)
			d->AddLine(scb1, scb2, IM_COL32(0, 0, 0, static_cast<int>(220.0f * layout.alpha)), cfg::esp::skeleton_thickness + 2.0f);
		d->AddLine(scb1, scb2, FadeColor(color, layout.alpha), cfg::esp::skeleton_thickness);
	}
}

void Esp::RenderPlayerChams(Player player, bool mate) {
    bool is_visible = cfg::esp::visible_check && player.spotted;
    auto color = mate ? 
        (is_visible ? cfg::esp::colors::chams_team_visible : cfg::esp::colors::chams_team) : 
        (is_visible ? cfg::esp::colors::chams_enemy_visible : cfg::esp::colors::chams_enemy);
    
    // Force 100% opacity to prevent alpha accumulation (double-blending) at the joints
    color.a = 1.0f;

    auto bone_count = player.bone_list.size();

    float distance = this->local.pos.dist_to(player.pos);
    if (distance < 1.0f) distance = 1.0f;

    // BaseThickness / Distance scaling
    float base_thickness = 6500.0f; // Increased base thickness to make them bulkier
    float thickness = base_thickness / distance;

    // Clamp to prevent blobbing out or being invisible
    if (thickness > 80.0f) thickness = 80.0f; // Increased max thickness
    if (thickness < 1.0f) thickness = 1.0f;

    // To create "fake chams", we'll draw thick lines/quads between bones to simulate volume
    for (const auto& bone : connections) {
        int first = bone[0], second = bone[1];

        if (bone_count <= first || bone_count <= second)
            continue;

        const auto& bone1 = player.bone_list[first];
        const auto& bone2 = player.bone_list[second];

        Vec2_t scb1;
        if (!matrix.wts(bone1.pos, io.DisplaySize, scb1))
            continue;

        Vec2_t scb2;
        if (!matrix.wts(bone2.pos, io.DisplaySize, scb2))
            continue;

        // Draw a thick line with lower opacity to simulate a colored overlay over the body part
        d->AddLine(
            scb1,
            scb2,
            ImColor(color),
            thickness // Distance scaled thickness
        );

        // Add rounded joints (capsules) to fix overlapping joints
        d->AddCircleFilled(
            scb1,
            thickness / 2.0f,
            ImColor(color),
            12
        );
        d->AddCircleFilled(
            scb2,
            thickness / 2.0f,
            ImColor(color),
            12
        );
    }
}

void Esp::RenderPlayerTracker(Player player, const PlayerLayout& layout) {
	if (player.bone_list.empty())
		return;

	auto head_bone = player.bone_list[bone_index::head];

	Vec2_t head;
	if (!matrix.wts(head_bone.pos, io.DisplaySize, head))
		return;

	auto width = layout.bounds.Width();
	auto color = layout.mate ?
        (layout.visible ? cfg::esp::colors::tracker_team_visible : cfg::esp::colors::tracker_team) :
        (layout.visible ? cfg::esp::colors::tracker_enemy_visible : cfg::esp::colors::tracker_enemy);

	d->AddCircleFilled(
		head,
		width / 6,
		FadeColor(color, layout.alpha),
		15
	);
}

void Esp::RenderPlayerEyeRay(Player player, const PlayerLayout& layout) {
    if (player.bone_list.empty())
        return;

    auto head_bone = player.bone_list[bone_index::head];

    Vec2_t head;
    if (!matrix.wts(head_bone.pos, io.DisplaySize, head))
        return;

    // Convert QAngle to forward vector
    float pitch = player.eye_angles.x * (3.14159265358979323846f / 180.0f);
    float yaw = player.eye_angles.y * (3.14159265358979323846f / 180.0f);
    
    Vec3_t forward;
    forward.x = cos(yaw) * cos(pitch);
    forward.y = sin(yaw) * cos(pitch);
    forward.z = -sin(pitch); // Source engine pitch is inverted

    // Ray length (adjust as needed)
    float length = 50.0f;
    Vec3_t ray_end_3d = head_bone.pos + (forward * length);

    Vec2_t ray_end;
    if (!matrix.wts(ray_end_3d, io.DisplaySize, ray_end))
        return;

    auto color = layout.mate ?
        (layout.visible ? cfg::esp::colors::eye_ray_team_visible : cfg::esp::colors::eye_ray_team) :
        (layout.visible ? cfg::esp::colors::eye_ray_enemy_visible : cfg::esp::colors::eye_ray_enemy);

    d->AddLine(
        head,
        ray_end,
        FadeColor(color, layout.alpha),
        cfg::esp::skeleton_thickness
    );
}

void Esp::RenderPlayerBars(Player player, const PlayerLayout& layout) {
	if (cfg::esp::health) {
		float health_frac = std::clamp(player.health / 100.0f, 0.0f, 1.0f);
		const ImU32 health_color = IM_COL32(
			static_cast<int>(255.0f - 155.0f * health_frac),
			static_cast<int>(50.0f + 205.0f * health_frac),
			50,
			static_cast<int>(255.0f * layout.alpha));
		esp_draw::AddBar(*d, layout.bounds, health_frac, health_color, esp_draw::BarSide::Left,
			cfg::esp::bar_thickness, cfg::esp::health_number && player.health < 100);
	}

	if (cfg::esp::armor) {
		float armor_frac = std::clamp(player.armor / 100.0f, 0.0f, 1.0f);
		esp_draw::AddBar(*d, layout.bounds, armor_frac,
			IM_COL32(90, 170, 255, static_cast<int>(255.0f * layout.alpha)),
			esp_draw::BarSide::Right, cfg::esp::bar_thickness, false);
	}
}

void Esp::RenderPlayerFalgs(Player player, const PlayerLayout& layout) {
	const auto bounds = std::pair<Vec2_t, Vec2_t>{
		{ layout.bounds.min.x, layout.bounds.min.y },
		{ layout.bounds.max.x, layout.bounds.max.y }
	};
	const bool mate = layout.mate;
	if (cfg::esp::flags::name) {
		auto sanitized_name = std::format("{}{}", player.name, (player.bot ? " (Bot)" : ""));
		auto name_size = ImGui::CalcTextSize(sanitized_name.data());

		d->AddText(
			Vec2_t(
				(bounds.first.x + bounds.second.x) / 2 - name_size.x / 2,
				bounds.first.y - 20
			), 
			IM_COL32(255, 255, 255, 255),
			sanitized_name.data()
		);
	}

	if (cfg::esp::flags::ammo && player.ammo != -1) {
		auto txt = std::to_string(player.ammo);
		auto ammo_size = ImGui::CalcTextSize(txt.c_str());

		d->AddText(
			Vec2_t(
				(bounds.first.x + bounds.second.x) / 2 - ammo_size.x / 2,
				bounds.second.y + 20
			),
			IM_COL32(255, 255, 255, 255),
			txt.data()
		);
	}

	int offset = 0;
	static int offset_mult = 15;

	if (cfg::esp::flags::money && player.money) {
		d->AddText(
			bounds.first - Vec2_t((bounds.first.x - bounds.second.x) - 10, offset),
			IM_COL32(255, 255, 255, 255),
			std::format("{}$", player.money).c_str()
		);

		offset -= offset_mult;
	}

	if (cfg::esp::flags::ping && player.ping) {
		d->AddText(
			bounds.first - Vec2_t((bounds.first.x - bounds.second.x) - 10, offset),
			IM_COL32(255, 255, 255, 255),
			std::format("{}ms", player.ping).c_str()
		);

		offset -= offset_mult;
	}

	if (cfg::esp::flags::distance) {
		int distance = (int)(this->local.pos.dist_to_3d(player.pos) * 0.0254f);
		d->AddText(
			bounds.first - Vec2_t((bounds.first.x - bounds.second.x) - 10, offset),
			IM_COL32(255, 255, 255, 255),
			std::format("{}m", distance).c_str()
		);

		offset -= offset_mult;
	}

	ImGui::PushFont(this->font_merged_icons);

	if (cfg::esp::flags::flashed && player.flashed || cfg::dev::force_show_flags) {
		auto color = mate ? cfg::esp::colors::flags::flashed_team : cfg::esp::colors::flags::flashed_enemy;

		d->AddText(
			bounds.first - Vec2_t((bounds.first.x - bounds.second.x) - 10, offset),
			ImColor(color),
			Icons::BLIND
		);

		offset -= offset_mult;
	}

	if (cfg::esp::flags::reloading && player.is_reloading || cfg::dev::force_show_flags) {
		auto color = mate ? cfg::esp::colors::flags::reloading_team : cfg::esp::colors::flags::reloading_enemy;

		d->AddText(
			bounds.first - Vec2_t((bounds.first.x - bounds.second.x) - 10, offset),
			ImColor(color),
			Icons::RELOAD
		);

		offset -= offset_mult;
	}

	if (cfg::esp::flags::defusing && player.defusing || cfg::dev::force_show_flags) {
		auto color = mate ? cfg::esp::colors::flags::defusing_team : cfg::esp::colors::flags::defusing_enemy;

		d->AddText(
			bounds.first - Vec2_t((bounds.first.x - bounds.second.x) - 10, offset),
			ImColor(color),
			WeaponIcons::CUTTERS
		);

		offset -= offset_mult;
	}

	if (cfg::esp::flags::scoped && player.scoped || cfg::dev::force_show_flags) {
		auto color = mate ? cfg::esp::colors::flags::scoped_team : cfg::esp::colors::flags::scoped_enemy;

		d->AddText(
			bounds.first - Vec2_t((bounds.first.x - bounds.second.x) - 10, offset),
			ImColor(color),
			WeaponIcons::SCOPE
		);

		offset -= offset_mult;
	}

	if (cfg::esp::flags::weapon) {
		auto weapon_size = ImGui::CalcTextSize(player.weapon.icon);

		d->AddText(
			Vec2_t(
				(bounds.first.x + bounds.second.x) / 2 - weapon_size.x / 2,
				bounds.second.y + 6
			),
			IM_COL32(255, 255, 255, 255),
			player.weapon.icon
		);
	}

	if (cfg::esp::flags::has_c4 && player.has_c4 || cfg::dev::force_show_flags) {
		auto color = mate ? cfg::esp::colors::flags::c4_team : cfg::esp::colors::flags::c4_enemy;

		ImGui::PushFont(this->font_merged_icons);
		auto icon_size = ImGui::CalcTextSize(WeaponIcons::C4);
		ImGui::PopFont();

		const ImColor draw_color(color.r, color.g, color.b, color.a * layout.alpha);

		d->AddText(
			this->font_merged_icons,
			16.0f,
			Vec2_t(
				(bounds.first.x + bounds.second.x) / 2 - icon_size.x / 2,
				bounds.first.y - 20 - icon_size.y - 2
			),
			draw_color,
			WeaponIcons::C4
		);

		offset -= offset_mult;
	}

	ImGui::PopFont();
}

void Esp::RenderCrosshair(Player local)
{
	if (!cfg::world::crosshair::enabled)
		return;

	if (local.scoped)
		return;

	auto weapon = local.weapon;

	if (weapon.item_index == -1)
		return;

	static std::vector<WeaponIds> valid_weapons = { weapon_ssg08, weapon_awp, weapon_g3sg1, weapon_scar20 };

	if (std::find(valid_weapons.begin(), valid_weapons.end(), weapon.item_index) == valid_weapons.end())
		return;

	ImVec2 center(
		floorf(io.DisplaySize.x * 0.5f),
		floorf(io.DisplaySize.y * 0.5f));

	constexpr float size = 6.f;
	constexpr float thickness = 1.0f;

	d->AddLine(
		ImVec2(center.x - size, center.y),
		ImVec2(center.x + size + 1, center.y),
		IM_COL32(255, 255, 255, 255),

		thickness);
	d->AddLine(
		ImVec2(center.x, center.y - size),
		ImVec2(center.x, center.y + size + 1),
		IM_COL32(255, 255, 255, 255),
		thickness);
}

void Esp::RenderPlayerTracers(Player source, Player player, const PlayerLayout& layout) {
	if (!cfg::esp::tracers)
		return;

	const ImVec2 screen_pos = esp_draw::ClampPoint(layout.projected, io.DisplaySize, 10.0f);
	auto color = layout.mate ?
        (layout.visible ? cfg::esp::colors::tracer_team_visible : cfg::esp::colors::tracer_team) :
        (layout.visible ? cfg::esp::colors::tracer_enemy_visible : cfg::esp::colors::tracer_enemy);

	d->AddLine(
		Vec2_t(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
		screen_pos,
		FadeColor(color, layout.alpha),
		1.0f
	);
}
