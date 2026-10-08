#include "Config.hpp"
#include "ConfigDocument.hpp"

namespace {

void ResetEspDefaults() {
	cfg::esp::team = true;
	cfg::esp::box = true;
	cfg::esp::armor = true;
	cfg::esp::health = true;
	cfg::esp::skeleton = true;
	cfg::esp::head_tracker = true;
	cfg::esp::health_number = false;
	cfg::esp::box_style = 0;
	cfg::esp::box_thickness = 1.0f;
	cfg::esp::outline = true;
	cfg::esp::offscreen_indicators = true;
	cfg::esp::fade_start = 45.0f;
	cfg::esp::fade_end = 110.0f;
	cfg::esp::max_distance = 140.0f;
	cfg::esp::skeleton_thickness = 1.5f;
	cfg::esp::text_scale = 1.0f;
	cfg::esp::bar_thickness = 2.0f;
	cfg::esp::spotted = false;
	cfg::esp::tracers = false;
	cfg::esp::chams = false;
	cfg::esp::eye_ray = false;
	cfg::esp::visible_check = false;

	cfg::esp::flags::name = true;
	cfg::esp::flags::ping = true;
	cfg::esp::flags::weapon = false;
	cfg::esp::flags::ammo = false;
	cfg::esp::flags::reloading = false;
	cfg::esp::flags::defusing = false;
	cfg::esp::flags::money = false;
	cfg::esp::flags::flashed = false;
	cfg::esp::flags::scoped = false;
	cfg::esp::flags::has_c4 = false;
	cfg::esp::flags::distance = false;

	cfg::esp::colors::box_team = { 0.f, 1.f, 0.29f, 0.5f };
	cfg::esp::colors::box_enemy = { 1.f, 0.f, 0.f, 0.5f };
	cfg::esp::colors::box_team_visible = { 0.f, 1.f, 0.29f, 0.8f };
	cfg::esp::colors::box_enemy_visible = { 1.f, 0.5f, 0.f, 0.8f };
	cfg::esp::colors::skeleton_team = { 0.f, 1.f, 0.f, 0.5f };
	cfg::esp::colors::skeleton_enemy = { 1.f, 0.f, 0.f, 0.5f };
	cfg::esp::colors::skeleton_team_visible = { 0.f, 1.f, 0.f, 0.8f };
	cfg::esp::colors::skeleton_enemy_visible = { 1.f, 0.5f, 0.f, 0.8f };
}

void ResetWorldDefaults() {
	cfg::world::spectators::enabled = false;
	cfg::world::spectators::detailed = false;
	cfg::world::spectators::self_only = true;
	cfg::world::spectators::pos = { 10.f, 100.f };
	cfg::world::bomb::location = true;
	cfg::world::bomb::timer = true;
	cfg::world::bomb::pos = { 10.f, 300.f };
	cfg::world::crosshair::enabled = false;
	cfg::world::radar::enabled = true;
	cfg::world::radar::no_rotate = false;
	cfg::world::radar::range = 2000.f;
	cfg::world::radar::pos = { 10.f, 10.f };
	cfg::world::radar::size = { 200.f, 200.f };
	cfg::world::velocity::enabled = false;
	cfg::world::velocity::sample_rate = 35;
	cfg::world::velocity::sample_length = 5.f;
	cfg::world::velocity::pos = { 10.f, 400.f };
	cfg::world::velocity::size = { 400.f, 100.f };
}

void ResetSettingsDefaults() {
	cfg::settings::watermark = true;
	cfg::settings::streamproof = false;
	cfg::settings::vsync = false;
	cfg::settings::free_cpu = true;
	cfg::settings::force_third_person = false;
	cfg::settings::defusal_notification = true;
}

} // namespace

bool Config::Read() {
	return GetInstance().ReadImpl();
}

bool Config::Write() {
	return GetInstance().WriteImpl();
}

const std::string& Config::LastStatus() {
	return GetInstance().last_status;
}

bool Config::ResetEsp() {
	ResetEspDefaults();
	return Write();
}

bool Config::ResetWorld() {
	ResetWorldDefaults();
	return Write();
}

bool Config::ResetSettings() {
	ResetSettingsDefaults();
	return Write();
}

bool Config::ResetAll() {
	cfg::enabled = true;
	ResetEspDefaults();
	ResetWorldDefaults();
	ResetSettingsDefaults();
	return Write();
}

bool Config::ReadImpl() {
	json data;
	std::string error;
	if (!std::filesystem::exists("config.json")) {
		last_status = "Configuration not found; defaults kept";
		return WriteImpl();
	}
	if (!config_document::Read("config.json", data, error)) {
		last_status = error;
		LOGF(WARNING, "{}", error);
		return false;
	}

	if (data.empty())
		return false;

	try {
		// general
		cfg::enabled = data.value("enabled", true);

		// esp
		cfg::esp::box = data["esp"].value("box", true);
		cfg::esp::team = data["esp"].value("team", true);
		cfg::esp::armor = data["esp"].value("armor", true);
		cfg::esp::health = data["esp"].value("health", true);
		cfg::esp::spotted = data["esp"].value("spotted", false);
		cfg::esp::skeleton = data["esp"].value("skeleton", true);
		cfg::esp::head_tracker = data["esp"].value("head_tracker", true);
		cfg::esp::health_number = data["esp"].value("health_number", false);
		cfg::esp::box_style = data["esp"].value("box_style", 0);
		cfg::esp::box_thickness = data["esp"].value("box_thickness", 1.0f);
		cfg::esp::outline = data["esp"].value("outline", true);
		cfg::esp::offscreen_indicators = data["esp"].value("offscreen_indicators", true);
		cfg::esp::fade_start = data["esp"].value("fade_start", 45.0f);
		cfg::esp::fade_end = data["esp"].value("fade_end", 110.0f);
		cfg::esp::max_distance = data["esp"].value("max_distance", 140.0f);
		cfg::esp::skeleton_thickness = data["esp"].value("skeleton_thickness", 1.5f);
		cfg::esp::text_scale = data["esp"].value("text_scale", 1.0f);
		cfg::esp::bar_thickness = data["esp"].value("bar_thickness", 2.0f);
		cfg::esp::tracers = data["esp"].value("tracers", false);
		cfg::esp::chams = data["esp"].value("chams", false);
		cfg::esp::eye_ray = data["esp"].value("eye_ray", false);
		cfg::esp::visible_check = data["esp"].value("visible_check", false);

		// flags
		cfg::esp::flags::name = data["esp"]["flags"].value("name", true);
		cfg::esp::flags::ping = data["esp"]["flags"].value("ping", false);
		cfg::esp::flags::money = data["esp"]["flags"].value("money", false);
		cfg::esp::flags::weapon = data["esp"]["flags"].value("weapon", false);
		cfg::esp::flags::ammo = data["esp"]["flags"].value("ammo", false);
		cfg::esp::flags::reloading = data["esp"]["flags"].value("reloading", false);
		cfg::esp::flags::scoped = data["esp"]["flags"].value("scoped", false);
		cfg::esp::flags::defusing = data["esp"]["flags"].value("defusing", false);
		cfg::esp::flags::flashed = data["esp"]["flags"].value("flashed", false);
		cfg::esp::flags::has_c4 = data["esp"]["flags"].value("has_c4", false);
		cfg::esp::flags::distance = data["esp"]["flags"].value("distance", false);

		// colors
		const auto& col = data["esp"]["colors"];
		cfg::esp::colors::box_team = JsonToColor(col, "box_team", { 0.f, 1.f, 0.29f, 0.5f });
		cfg::esp::colors::box_enemy = JsonToColor(col, "box_enemy", { 1.f, 0.f, 0.f, 0.5f });
		cfg::esp::colors::box_team_visible = JsonToColor(col, "box_team_visible", { 0.f, 1.f, 0.29f, 0.8f });
		cfg::esp::colors::box_enemy_visible = JsonToColor(col, "box_enemy_visible", { 1.f, 0.5f, 0.f, 0.8f });

		cfg::esp::colors::skeleton_team = JsonToColor(col, "skeleton_team", { 0.f, 1.f, 0.f, 0.5f });
		cfg::esp::colors::skeleton_enemy = JsonToColor(col, "skeleton_enemy", { 1.f, 0.f, 0.f, 0.5f });
		cfg::esp::colors::skeleton_team_visible = JsonToColor(col, "skeleton_team_visible", { 0.f, 1.f, 0.f, 0.8f });
		cfg::esp::colors::skeleton_enemy_visible = JsonToColor(col, "skeleton_enemy_visible", { 1.f, 0.5f, 0.f, 0.8f });

		cfg::esp::colors::tracker_team = JsonToColor(col, "tracker_team", { 1.f, 1.f, 1.f, 0.3f });
		cfg::esp::colors::tracker_enemy = JsonToColor(col, "tracker_enemy", { 1.f, 1.f, 1.f, 0.3f });
		cfg::esp::colors::tracker_team_visible = JsonToColor(col, "tracker_team_visible", { 1.f, 1.f, 1.f, 0.8f });
		cfg::esp::colors::tracker_enemy_visible = JsonToColor(col, "tracker_enemy_visible", { 1.f, 0.5f, 0.f, 0.8f });

		cfg::esp::colors::tracer_team = JsonToColor(col, "tracer_team", { 0.f, 1.f, 0.f, 0.5f });
		cfg::esp::colors::tracer_enemy = JsonToColor(col, "tracer_enemy", { 1.f, 0.f, 0.f, 0.5f });
		cfg::esp::colors::tracer_team_visible = JsonToColor(col, "tracer_team_visible", { 0.f, 1.f, 0.f, 0.8f });
		cfg::esp::colors::tracer_enemy_visible = JsonToColor(col, "tracer_enemy_visible", { 1.f, 0.5f, 0.f, 0.8f });

		cfg::esp::colors::chams_team = JsonToColor(col, "chams_team", { 0.f, 1.f, 0.f, 0.2f });
		cfg::esp::colors::chams_enemy = JsonToColor(col, "chams_enemy", { 1.f, 0.f, 0.f, 0.2f });
		cfg::esp::colors::chams_team_visible = JsonToColor(col, "chams_team_visible", { 0.f, 1.f, 0.f, 0.5f });
		cfg::esp::colors::chams_enemy_visible = JsonToColor(col, "chams_enemy_visible", { 1.f, 0.5f, 0.f, 0.5f });

		cfg::esp::colors::eye_ray_team = JsonToColor(col, "eye_ray_team", { 0.f, 1.f, 1.f, 0.5f });
		cfg::esp::colors::eye_ray_enemy = JsonToColor(col, "eye_ray_enemy", { 1.f, 1.f, 0.f, 0.5f });
		cfg::esp::colors::eye_ray_team_visible = JsonToColor(col, "eye_ray_team_visible", { 0.f, 1.f, 1.f, 0.8f });
		cfg::esp::colors::eye_ray_enemy_visible = JsonToColor(col, "eye_ray_enemy_visible", { 1.f, 1.f, 0.f, 0.8f });

		// flag colors
		const auto& fcol = data["esp"]["colors"]["flags"];

		cfg::esp::colors::flags::flashed_team = JsonToColor(fcol, "flashed_team", { 1.f, 1.f, 1.f, 0.5f });
		cfg::esp::colors::flags::flashed_enemy = JsonToColor(fcol, "flashed_enemy", { 1.f, 1.f, 1.f, 0.8f });

		cfg::esp::colors::flags::reloading_team = JsonToColor(fcol, "reloading_team", { 1.f, 1.f, 1.f, 0.5f });
		cfg::esp::colors::flags::reloading_enemy = JsonToColor(fcol, "reloading_enemy", { 1.f, 1.f, 1.f, 0.8f });

		cfg::esp::colors::flags::defusing_team = JsonToColor(fcol, "defusing_team", { 1.f, 1.f, 1.f, 0.5f });
		cfg::esp::colors::flags::defusing_enemy = JsonToColor(fcol, "defusing_enemy", { 1.f, 1.f, 1.f, 0.8f });

		cfg::esp::colors::flags::scoped_team = JsonToColor(fcol, "scoped_team", { 1.f, 1.f, 1.f, 0.5f });
		cfg::esp::colors::flags::scoped_enemy = JsonToColor(fcol, "scoped_enemy", { 1.f, 1.f, 1.f, 0.8f });

		cfg::esp::colors::flags::c4_team = JsonToColor(fcol, "c4_team", { 1.f, 0.84f, 0.f, 1.f });
		cfg::esp::colors::flags::c4_enemy = JsonToColor(fcol, "c4_enemy", { 1.f, 0.84f, 0.f, 1.f });

		// world
		// spectator list
		cfg::world::spectators::enabled = data["world"]["spectators"].value("enabled", true);
		cfg::world::spectators::detailed = data["world"]["spectators"].value("detailed", false);
		cfg::world::spectators::self_only = data["world"]["spectators"].value("self_only", true);
		cfg::world::spectators::pos = JsonToVec2(data["world"]["spectators"], "pos", {10.f, 100.f});

		// bomb
		cfg::world::bomb::location = data["world"]["bomb"].value("location", true);
		cfg::world::bomb::timer = data["world"]["bomb"].value("timer", true);
		cfg::world::bomb::pos = JsonToVec2(data["world"]["bomb"], "pos", { 10.f, 300.f });

		// crosshair
		cfg::world::crosshair::enabled = data["world"]["crosshair"].value("enabled", false); 

		// radar
		cfg::world::radar::enabled = data["world"]["radar"].value("enabled", true);
		cfg::world::radar::no_rotate = data["world"]["radar"].value("no_rotate", false);
		cfg::world::radar::range = data["world"]["radar"].value("range", 2000.f);
		cfg::world::radar::pos = JsonToVec2(data["world"]["radar"], "pos", { 10.f, 10.f });
		cfg::world::radar::size = JsonToVec2(data["world"]["radar"], "size", { 200.f, 200.f });

		// velocity
		cfg::world::velocity::enabled = data["world"]["velocity"].value("enabled", false);
		cfg::world::velocity::sample_rate = data["world"]["velocity"].value("sample_rate", 10);
		cfg::world::velocity::sample_length = data["world"]["velocity"].value("sample_length", 5.f);
		cfg::world::velocity::pos = JsonToVec2(data["world"]["velocity"], "pos", { 10.f, 400.f });
		cfg::world::velocity::size = JsonToVec2(data["world"]["velocity"], "size", { 400.f, 100.f });

		// utils
		//cfg::settings::console = data["utils"].value("console", true);
		cfg::settings::watermark = data["utils"].value("watermark", true);
		cfg::settings::streamproof = data["utils"].value("streamproof", false);
		cfg::settings::vsync = data["utils"].value("vsync", true);
		cfg::settings::free_cpu = data["utils"].value("free_cpu", true);
		cfg::settings::force_third_person = data["utils"].value("force_third_person", false);
		cfg::settings::defusal_notification = data["utils"].value("defusal_notification", true);
		//cfg::settings::open_menu_key = data["utils"].value("open_menu_key", 0);
	}
	catch (const std::exception& e) {
		last_status = std::string("Failed to apply configuration: ") + e.what();
		LOGF(WARNING, "{}", last_status);
		return false;
	}

	last_status = "Configuration loaded";
	LOGF(INFO, "Successfully parsed configuration");
	return true;
}

bool Config::WriteImpl() {
	json data = json::object();
	std::string error;
	if (std::filesystem::exists("config.json") && !config_document::Read("config.json", data, error)) {
		last_status = "Save blocked to preserve invalid config: " + error;
		return false;
	}

	data["enabled"] = cfg::enabled;
	data["esp"]["box"] = cfg::esp::box;
	data["esp"]["team"] = cfg::esp::team;
	data["esp"]["armor"] = cfg::esp::armor;
	data["esp"]["health"] = cfg::esp::health;
	data["esp"]["spotted"] = cfg::esp::spotted;
	data["esp"]["skeleton"] = cfg::esp::skeleton;
	data["esp"]["head_tracker"] = cfg::esp::head_tracker;
	data["esp"]["health_number"] = cfg::esp::health_number;
	data["esp"]["box_style"] = cfg::esp::box_style;
	data["esp"]["box_thickness"] = cfg::esp::box_thickness;
	data["esp"]["outline"] = cfg::esp::outline;
	data["esp"]["offscreen_indicators"] = cfg::esp::offscreen_indicators;
	data["esp"]["fade_start"] = cfg::esp::fade_start;
	data["esp"]["fade_end"] = cfg::esp::fade_end;
	data["esp"]["max_distance"] = cfg::esp::max_distance;
	data["esp"]["skeleton_thickness"] = cfg::esp::skeleton_thickness;
	data["esp"]["text_scale"] = cfg::esp::text_scale;
    data["esp"]["bar_thickness"] = cfg::esp::bar_thickness;
	data["esp"]["tracers"] = cfg::esp::tracers;
	data["esp"]["chams"] = cfg::esp::chams;
	data["esp"]["eye_ray"] = cfg::esp::eye_ray;
	data["esp"]["visible_check"] = cfg::esp::visible_check;

	data["esp"]["flags"]["name"] = cfg::esp::flags::name;
	data["esp"]["flags"]["ping"] = cfg::esp::flags::ping;
	data["esp"]["flags"]["money"] = cfg::esp::flags::money;
	data["esp"]["flags"]["weapon"] = cfg::esp::flags::weapon;
	data["esp"]["flags"]["ammo"] = cfg::esp::flags::ammo;
	data["esp"]["flags"]["reloading"] = cfg::esp::flags::reloading;
	data["esp"]["flags"]["scoped"] = cfg::esp::flags::scoped;
	data["esp"]["flags"]["defusing"] = cfg::esp::flags::defusing;
	data["esp"]["flags"]["flashed"] = cfg::esp::flags::flashed;
	data["esp"]["flags"]["has_c4"] = cfg::esp::flags::has_c4;
	data["esp"]["flags"]["distance"] = cfg::esp::flags::distance;

	ColorToJson(data["esp"]["colors"], "box_team", cfg::esp::colors::box_team);
	ColorToJson(data["esp"]["colors"], "box_enemy", cfg::esp::colors::box_enemy);
	ColorToJson(data["esp"]["colors"], "box_team_visible", cfg::esp::colors::box_team_visible);
	ColorToJson(data["esp"]["colors"], "box_enemy_visible", cfg::esp::colors::box_enemy_visible);

	ColorToJson(data["esp"]["colors"], "skeleton_team", cfg::esp::colors::skeleton_team);
	ColorToJson(data["esp"]["colors"], "skeleton_enemy", cfg::esp::colors::skeleton_enemy);
	ColorToJson(data["esp"]["colors"], "skeleton_team_visible", cfg::esp::colors::skeleton_team_visible);
	ColorToJson(data["esp"]["colors"], "skeleton_enemy_visible", cfg::esp::colors::skeleton_enemy_visible);

	ColorToJson(data["esp"]["colors"], "tracker_team", cfg::esp::colors::tracker_team);
	ColorToJson(data["esp"]["colors"], "tracker_enemy", cfg::esp::colors::tracker_enemy);
	ColorToJson(data["esp"]["colors"], "tracker_team_visible", cfg::esp::colors::tracker_team_visible);
	ColorToJson(data["esp"]["colors"], "tracker_enemy_visible", cfg::esp::colors::tracker_enemy_visible);

	ColorToJson(data["esp"]["colors"], "tracer_team", cfg::esp::colors::tracer_team);
	ColorToJson(data["esp"]["colors"], "tracer_enemy", cfg::esp::colors::tracer_enemy);
	ColorToJson(data["esp"]["colors"], "tracer_team_visible", cfg::esp::colors::tracer_team_visible);
	ColorToJson(data["esp"]["colors"], "tracer_enemy_visible", cfg::esp::colors::tracer_enemy_visible);

	ColorToJson(data["esp"]["colors"], "chams_team", cfg::esp::colors::chams_team);
	ColorToJson(data["esp"]["colors"], "chams_enemy", cfg::esp::colors::chams_enemy);
	ColorToJson(data["esp"]["colors"], "chams_team_visible", cfg::esp::colors::chams_team_visible);
	ColorToJson(data["esp"]["colors"], "chams_enemy_visible", cfg::esp::colors::chams_enemy_visible);

	ColorToJson(data["esp"]["colors"], "eye_ray_team", cfg::esp::colors::eye_ray_team);
	ColorToJson(data["esp"]["colors"], "eye_ray_enemy", cfg::esp::colors::eye_ray_enemy);
	ColorToJson(data["esp"]["colors"], "eye_ray_team_visible", cfg::esp::colors::eye_ray_team_visible);
	ColorToJson(data["esp"]["colors"], "eye_ray_enemy_visible", cfg::esp::colors::eye_ray_enemy_visible);

	// spectator list
	data["world"]["spectators"]["enabled"] = cfg::world::spectators::enabled;
	data["world"]["spectators"]["detailed"] = cfg::world::spectators::detailed;
	data["world"]["spectators"]["self_only"] = cfg::world::spectators::self_only;
	Vec2ToJson(data["world"]["spectators"], "pos", cfg::world::spectators::pos);

	// bomb
	data["world"]["bomb"]["location"] = cfg::world::bomb::location;
	data["world"]["bomb"]["timer"] = cfg::world::bomb::timer;
	Vec2ToJson(data["world"]["bomb"], "pos", cfg::world::bomb::pos);

	// crosshair
	data["world"]["crosshair"]["enabled"] = cfg::world::crosshair::enabled;

	// radar
	data["world"]["radar"]["enabled"] = cfg::world::radar::enabled;
	data["world"]["radar"]["no_rotate"] = cfg::world::radar::no_rotate;
	data["world"]["radar"]["range"] = cfg::world::radar::range;
	Vec2ToJson(data["world"]["radar"], "pos", cfg::world::radar::pos);
	Vec2ToJson(data["world"]["radar"], "size", cfg::world::radar::size);

	// velocity
	data["world"]["velocity"]["enabled"] = cfg::world::velocity::enabled;
	data["world"]["velocity"]["sample_rate"] = cfg::world::velocity::sample_rate;
	data["world"]["velocity"]["sample_length"] = cfg::world::velocity::sample_length;
	Vec2ToJson(data["world"]["velocity"], "pos", cfg::world::velocity::pos);
	Vec2ToJson(data["world"]["velocity"], "size", cfg::world::velocity::size);

	// colors
	auto& col = data["esp"]["colors"];
	ColorToJson(col, "box_team", cfg::esp::colors::box_team);
	ColorToJson(col, "box_enemy", cfg::esp::colors::box_enemy);

	ColorToJson(col, "skeleton_team", cfg::esp::colors::skeleton_team);
	ColorToJson(col, "skeleton_enemy", cfg::esp::colors::skeleton_enemy);

	ColorToJson(col, "tracker_team", cfg::esp::colors::tracker_team);
	ColorToJson(col, "tracker_enemy", cfg::esp::colors::tracker_enemy);

	ColorToJson(col, "tracer_team", cfg::esp::colors::tracer_team);
	ColorToJson(col, "tracer_enemy", cfg::esp::colors::tracer_enemy);

	// flag colors
	auto& fcol = col["flags"];

	ColorToJson(fcol, "flashed_team", cfg::esp::colors::flags::flashed_team);
	ColorToJson(fcol, "flashed_enemy", cfg::esp::colors::flags::flashed_enemy);

	ColorToJson(fcol, "reloading_team", cfg::esp::colors::flags::reloading_team);
	ColorToJson(fcol, "reloading_enemy", cfg::esp::colors::flags::reloading_enemy);

	ColorToJson(fcol, "defusing_team", cfg::esp::colors::flags::defusing_team);
	ColorToJson(fcol, "defusing_enemy", cfg::esp::colors::flags::defusing_enemy);

	ColorToJson(fcol, "scoped_team", cfg::esp::colors::flags::scoped_team);
	ColorToJson(fcol, "scoped_enemy", cfg::esp::colors::flags::scoped_enemy);

	ColorToJson(fcol, "c4_team", cfg::esp::colors::flags::c4_team);
	ColorToJson(fcol, "c4_enemy", cfg::esp::colors::flags::c4_enemy);

	// utils
	//data["utils"]["console"] = cfg::settings::console;
	data["utils"]["watermark"] = cfg::settings::watermark;
	data["utils"]["streamproof"] = cfg::settings::streamproof;
	data["utils"]["vsync"] = cfg::settings::vsync;
	data["utils"]["free_cpu"] = cfg::settings::free_cpu;
	data["utils"]["force_third_person"] = cfg::settings::force_third_person;
	data["utils"]["defusal_notification"] = cfg::settings::defusal_notification;
	//data["utils"]["open_menu_key"] = cfg::settings::open_menu_key;

	if (!config_document::WriteAtomic("config.json", data, error)) {
		last_status = error;
		LOGF(WARNING, "{}", error);
		return false;
	}

	last_status = "Configuration saved";
	LOGF(VERBOSE, "Writting configuration to file");

	return true;
}


// TODO: Refactor this
color_t Config::JsonToColor(const json& parent, const std::string& key, const color_t& def) {
	if (!parent.contains(key) || !parent[key].is_array() || parent[key].size() != 4)
		return def;
	return color_t(
		parent[key][0].get<float>(),
		parent[key][1].get<float>(),
		parent[key][2].get<float>(),
		parent[key][3].get<float>()
	);
}

void Config::ColorToJson(json& parent, const std::string& key, const color_t& color) {
	parent[key] = { color.r, color.g, color.b, color.a };
}

Vec2_t Config::JsonToVec2(const json& parent, const std::string& key, const Vec2_t& def)
{
	if (!parent.contains(key) || !parent[key].is_array() || parent[key].size() != 2)
		return def;

	return Vec2_t{
		parent[key][0].get<float>(),
		parent[key][1].get<float>()
	};
}

void Config::Vec2ToJson(json& parent, const std::string& key, const Vec2_t& vec)
{
	parent[key] = { vec.x, vec.y };
}
