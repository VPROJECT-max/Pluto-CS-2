#pragma once

#include "core/engine/cache/Cache.hpp"
#include "gui/frontend/esp/DrawPrimitives.hpp"

class Esp {
public:
    ~Esp() = default;
    Esp(const Esp&) = delete;
    Esp(Esp&&) = delete;
    Esp& operator=(const Esp&) = delete;
    Esp& operator=(Esp&&) = delete;

    static bool Init();
    static void Render();

private:
	struct PlayerLayout {
		esp_draw::ScreenRect bounds{};
		ImVec2 projected{};
		float distance_m = 0.0f;
		float alpha = 1.0f;
		bool on_screen = false;
		bool visible = false;
		bool mate = false;
	};

    ImGuiIO io;
    ImFont* font;
    ImFont* font_merged_icons;
    ImDrawList* d;

    // Temporary storage for ease
    view_matrix_t matrix;
    Player local;
private:
    Esp() {};

    static Esp& GetInstance()
    {
        static Esp i{};
        return i;
    }

    bool InitImpl();
    void RenderImpl();

	bool BuildPlayerLayout(Player player, bool mate, PlayerLayout& layout);
	void RenderPlayer(Player player, const PlayerLayout& layout);
	void RenderPlayerBones(Player player, const PlayerLayout& layout);
	void RenderPlayerChams(Player player, bool mate = false);
	void RenderPlayerBars(Player player, const PlayerLayout& layout);
	void RenderPlayerFalgs(Player player, const PlayerLayout& layout);
	void RenderPlayerTracker(Player player, const PlayerLayout& layout);
	void RenderPlayerEyeRay(Player player, const PlayerLayout& layout);
	void RenderPlayerTracers(Player source, Player player, const PlayerLayout& layout);

	void RenderCrosshair(Player local);
};
