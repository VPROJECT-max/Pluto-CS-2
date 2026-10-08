#pragma once

namespace cfg {
	inline bool enabled = true;

	namespace esp {
		inline bool team = true;

		inline bool box = true;
		inline bool armor = true;
		inline bool health = true;
		inline bool skeleton = true;
		inline bool head_tracker = true;
		inline bool health_number = false;
		inline int box_style = 0; // 0 = full, 1 = corner
		inline float box_thickness = 1.0f;
		inline bool outline = true;
		inline bool offscreen_indicators = true;
		inline float fade_start = 45.0f;
		inline float fade_end = 110.0f;
		inline float max_distance = 140.0f;
		inline float skeleton_thickness = 1.5f;
		inline float text_scale = 1.0f;
		inline float bar_thickness = 2.0f;

		inline bool spotted = false;

		inline bool tracers = false;
		inline bool chams = false;
		inline bool eye_ray = false;
		inline bool visible_check = false;

		namespace flags {
			inline bool name = true;
			inline bool ping = true;
			inline bool weapon = false;
			inline bool ammo = false;
			inline bool reloading = false;
			inline bool defusing = false;
			inline bool money = false;
			inline bool flashed = false;
			inline bool scoped = false;
			inline bool has_c4 = false;
			inline bool distance = false;
		}

		namespace colors {
			inline color_t box_team{ 0.f, 1.f, 0.29f, 0.5f };
			inline color_t box_enemy{ 1.f, 0.f, 0.f, 0.5f };
			inline color_t box_team_visible{ 0.f, 1.f, 0.29f, 0.8f };
			inline color_t box_enemy_visible{ 1.f, 0.5f, 0.f, 0.8f };

			inline color_t skeleton_team{ 0.f, 1.f, 0.f, 0.5f };
			inline color_t skeleton_enemy{ 1.f, 0.f, 0.f, 0.5f };
			inline color_t skeleton_team_visible{ 0.f, 1.f, 0.f, 0.8f };
			inline color_t skeleton_enemy_visible{ 1.f, 0.5f, 0.f, 0.8f };

			inline color_t tracker_team{ 1.f, 1.f, 1.f, 0.3f };
			inline color_t tracker_enemy{ 1.f, 1.f, 1.f, 0.3f };
			inline color_t tracker_team_visible{ 1.f, 1.f, 1.f, 0.8f };
			inline color_t tracker_enemy_visible{ 1.f, 0.5f, 0.f, 0.8f };

			inline color_t tracer_team{ 0.f, 1.f, 0.f, 0.5f };
			inline color_t tracer_enemy{ 1.f, 0.f, 0.f, 0.5f };
			inline color_t tracer_team_visible{ 0.f, 1.f, 0.f, 0.8f };
			inline color_t tracer_enemy_visible{ 1.f, 0.5f, 0.f, 0.8f };

			inline color_t chams_team{ 0.f, 1.f, 0.f, 0.7f };
			inline color_t chams_enemy{ 1.f, 0.f, 0.f, 0.7f };
			inline color_t chams_team_visible{ 0.f, 1.f, 0.f, 1.0f };
			inline color_t chams_enemy_visible{ 1.f, 0.5f, 0.f, 1.0f };

			inline color_t eye_ray_team{ 0.f, 1.f, 1.f, 0.5f };
			inline color_t eye_ray_enemy{ 1.f, 1.f, 0.f, 0.5f };
			inline color_t eye_ray_team_visible{ 0.f, 1.f, 1.f, 0.8f };
			inline color_t eye_ray_enemy_visible{ 1.f, 1.f, 0.f, 0.8f };

			namespace flags {
				inline color_t flashed_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t flashed_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t reloading_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t reloading_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t defusing_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t defusing_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t scoped_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t scoped_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t c4_team{ 1.f, 0.84f, 0.f, 1.f };
				inline color_t c4_enemy{ 1.f, 0.84f, 0.f, 1.f };
			}
			
		}

	}

	namespace world {
		namespace spectators {
			inline bool enabled = false;

			inline bool detailed = false;
			inline bool self_only = true;

			inline Vec2_t pos{ 10.f, 100.f };
		}

		namespace bomb {
			inline bool location = true;
			inline bool timer = true;
			inline Vec2_t pos{ 10.f, 300.f };
		}

		namespace crosshair {
			inline bool enabled = false;
		}

		namespace radar {
			inline bool enabled = true;
			inline bool no_rotate = false;
			inline float range = 2000.f;
			inline Vec2_t pos{ 10.f, 10.f };
			inline Vec2_t size{ 200.f, 200.f };
		}

		namespace velocity {
			inline bool enabled = false;
			inline int sample_rate = 35;
			inline float sample_length = 5.f;

			inline Vec2_t size{ 400.f, 100.f };
			inline Vec2_t pos{ 10.f, 400.f };
		}
	}

	namespace settings {
		inline bool watermark = true;
		inline bool streamproof = false;
		inline bool vsync = false;
		inline bool free_cpu = true;
		inline bool force_third_person = false;
		inline bool defusal_notification = true;
		inline bool debug_overlay = false;
	}


	// Not stored, just for testing
	namespace dev {
		inline bool console = true;
		inline int open_menu_key = false;
		inline int cache_refresh_rate = 2;
		inline bool force_show_flags = false;
	}
}
