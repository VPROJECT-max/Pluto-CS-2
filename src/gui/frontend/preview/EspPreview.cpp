#include "EspPreview.hpp"

#include "assets/images/EspPreviewImage.hpp"
#include "assets/images/ImageLoader.hpp"
#include "config/Current.hpp"
#include "gui/frontend/esp/DrawPrimitives.hpp"
#include "gui/frontend/overlays/OverlayPresentation.hpp"
#include "gui/theme/Theme.hpp"

#include <wrl/client.h>

namespace {

ImU32 PreviewColor(const color_t& color, float alpha) {
    return ImColor(color.r, color.g, color.b, color.a * alpha);
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> preview_texture;
int preview_width{};
int preview_height{};

} // namespace

bool EspPreview::Initialize(ID3D11Device* device) {
    if (preview_texture || device == nullptr)
        return preview_texture != nullptr;

    ID3D11ShaderResourceView* loaded_texture = nullptr;
    if (!ImageLoader::LoadTextureFromMemory(
            device,
            esp_preview_png,
            esp_preview_png_len,
            &loaded_texture,
            &preview_width,
            &preview_height)) {
        return false;
    }
    preview_texture.Attach(loaded_texture);
    return true;
}

void EspPreview::Shutdown() {
    preview_texture.Reset();
    preview_width = 0;
    preview_height = 0;
}

void EspPreview::Render(ImVec2 size) {
    size.x = (std::max)(size.x, 220.0f);
    size.y = (std::max)(size.y, 360.0f);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##esp_preview", size);

    ImDrawList& draw = *ImGui::GetWindowDrawList();
    const ImVec2 end{ origin.x + size.x, origin.y + size.y };
    draw.AddRectFilled(origin, end, ImGui::GetColorU32(ui::theme::colors::Canvas), 6.0f);
    if (preview_texture && preview_width > 0 && preview_height > 0) {
        const auto crop = overlay_presentation::CalculateCoverCrop(
            { static_cast<float>(preview_width), static_cast<float>(preview_height) },
            { size.x, size.y });
        draw.PushClipRect(origin, end, true);
        draw.AddImage(
            preview_texture.Get(),
            origin,
            end,
            { crop.uv_min.x, crop.uv_min.y },
            { crop.uv_max.x, crop.uv_max.y });
        draw.AddRectFilled(origin, end, IM_COL32(8, 8, 10, 30));
        draw.PopClipRect();
    }
    draw.AddRect(origin, end, ImGui::GetColorU32(ui::theme::colors::Border), 6.0f);

    constexpr float distance_m = 64.0f;
    const float alpha = esp_draw::DistanceAlpha(distance_m, cfg::esp::fade_start, cfg::esp::fade_end);
    const esp_draw::ScreenRect player{
        { origin.x + size.x * 0.10f, origin.y + size.y * 0.035f },
        { origin.x + size.x * 0.90f, origin.y + size.y * 0.93f },
    };
    const color_t box_color = cfg::esp::visible_check
        ? cfg::esp::colors::box_enemy_visible : cfg::esp::colors::box_enemy;

    if (cfg::esp::box) {
        if (cfg::esp::box_style == 1)
            esp_draw::AddCornerBox(draw, player, PreviewColor(box_color, alpha), cfg::esp::box_thickness, 0.25f, cfg::esp::outline);
        else
            esp_draw::AddBox(draw, player, PreviewColor(box_color, alpha), cfg::esp::box_thickness, cfg::esp::outline);
    }

    if (cfg::esp::skeleton) {
        const ImU32 skeleton = PreviewColor(
            cfg::esp::visible_check ? cfg::esp::colors::skeleton_enemy_visible : cfg::esp::colors::skeleton_enemy,
            alpha);
        const ImVec2 head{ (player.min.x + player.max.x) * 0.5f, player.min.y + player.Height() * 0.12f };
        const ImVec2 chest{ head.x, player.min.y + player.Height() * 0.34f };
        const ImVec2 hips{ head.x, player.min.y + player.Height() * 0.57f };
        const ImVec2 left_hand{ player.min.x + player.Width() * 0.08f, player.min.y + player.Height() * 0.48f };
        const ImVec2 right_hand{ player.max.x - player.Width() * 0.08f, left_hand.y };
        const ImVec2 left_foot{ player.min.x + player.Width() * 0.25f, player.max.y };
        const ImVec2 right_foot{ player.max.x - player.Width() * 0.25f, player.max.y };
        const auto line = [&](ImVec2 a, ImVec2 b) {
            if (cfg::esp::outline)
                draw.AddLine(a, b, IM_COL32(0, 0, 0, 220), cfg::esp::skeleton_thickness + 2.0f);
            draw.AddLine(a, b, skeleton, cfg::esp::skeleton_thickness);
        };
        line(head, chest); line(chest, hips); line(chest, left_hand); line(chest, right_hand);
        line(hips, left_foot); line(hips, right_foot);
    }

    if (cfg::esp::health)
        esp_draw::AddBar(draw, player, 0.78f, IM_COL32(112, 224, 126, static_cast<int>(255.0f * alpha)),
            esp_draw::BarSide::Left, cfg::esp::bar_thickness, cfg::esp::health_number);
    if (cfg::esp::armor)
        esp_draw::AddBar(draw, player, 0.62f, IM_COL32(90, 170, 255, static_cast<int>(255.0f * alpha)),
            esp_draw::BarSide::Right, cfg::esp::bar_thickness, false);

    const float text_size = 12.0f * std::clamp(cfg::esp::text_scale, 0.75f, 1.5f);
    if (cfg::esp::flags::name)
        esp_draw::AddOutlinedText(draw, nullptr, text_size, { (player.min.x + player.max.x) * 0.5f, player.min.y - 18.0f },
            IM_COL32(243, 243, 243, static_cast<int>(255.0f * alpha)), "ENEMY", esp_draw::TextAlign::Center);
    if (cfg::esp::flags::distance)
        esp_draw::AddOutlinedText(draw, nullptr, text_size, { player.max.x + 10.0f, player.min.y },
            IM_COL32(200, 200, 200, static_cast<int>(255.0f * alpha)), "64m");

    if (cfg::esp::offscreen_indicators) {
        draw.PushClipRect(origin, end, true);
        esp_draw::AddOffscreenIndicator(draw, end, { end.x + 80.0f, origin.y + size.y * 0.34f }, distance_m,
            PreviewColor(box_color, alpha), 24.0f);
        draw.PopClipRect();
    }
}
