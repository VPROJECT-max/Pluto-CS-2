#include "PlutoBrand.hpp"

#include "assets/fonts/PlutoWordmarkFont.hpp"
#include "assets/images/ImageLoader.hpp"
#include "assets/images/PlutoLogoImage.hpp"
#include "core/version/AppVersion.hpp"

#include <d3d11.h>
#include <imgui.h>

#include <cfloat>
#include <string>

namespace PlutoBrand {
namespace {

ID3D11ShaderResourceView* logo_texture{};
ImFont* wordmark_font{};
int logo_width{};
int logo_height{};

} // namespace

bool Initialize(ID3D11Device* device, ImFontAtlas& fonts) {
    Shutdown();
    if (device == nullptr) {
        return false;
    }

    ImFontConfig config{};
    config.FontDataOwnedByAtlas = false;
    static const ImWchar glyph_ranges[]{ 0x20, 0x7E, 0 };
    wordmark_font = fonts.AddFontFromMemoryTTF(
        const_cast<unsigned char*>(pluto_assets::PlutoWordmarkTtf),
        static_cast<int>(pluto_assets::PlutoWordmarkTtfSize),
        21.0f,
        &config,
        glyph_ranges);
    if (wordmark_font == nullptr) {
        return false;
    }

    if (!ImageLoader::LoadTextureFromMemory(
        device,
        pluto_assets::PlutoLogoPng,
        static_cast<int>(pluto_assets::PlutoLogoPngSize),
        &logo_texture,
        &logo_width,
        &logo_height)) {
        wordmark_font = nullptr;
        return false;
    }
    return true;
}

void Shutdown() {
    if (logo_texture != nullptr) {
        logo_texture->Release();
        logo_texture = nullptr;
    }
    wordmark_font = nullptr;
    logo_width = 0;
    logo_height = 0;
}

void RenderHeaderLockup(ImDrawList* draw_list, const Rect& header) {
    if (draw_list == nullptr || logo_texture == nullptr || wordmark_font == nullptr) {
        return;
    }

    constexpr char wordmark[]{ "Pluto" };
    const std::string version = "v" + std::string{ app_version::current_text };
    const ImVec2 wordmark_size = wordmark_font->CalcTextSizeA(
        wordmark_font->LegacySize, FLT_MAX, 0.0f, wordmark);
    ImFont* version_font = ImGui::GetFont();
    const float version_font_size = 12.0f;
    const ImVec2 version_size = version_font->CalcTextSizeA(
        version_font_size, FLT_MAX, 0.0f, version.c_str());
    const auto layout = CalculateLayout(
        header,
        { 38.0f, 38.0f },
        { wordmark_size.x, wordmark_size.y },
        { version_size.x, version_size.y });

    if (layout.show_wordmark) {
        draw_list->AddText(
            wordmark_font,
            wordmark_font->LegacySize,
            { layout.wordmark.min_x, layout.wordmark.min_y },
            IM_COL32(238, 236, 245, 255),
            wordmark);
    }
    if (layout.show_version) {
        draw_list->AddText(
            version_font,
            version_font_size,
            { layout.version.min_x, layout.version.min_y },
            IM_COL32(135, 132, 146, 255),
            version.c_str());
    }
    draw_list->AddImage(
        logo_texture,
        { layout.logo.min_x, layout.logo.min_y },
        { layout.logo.max_x, layout.logo.max_y });
}

} // namespace PlutoBrand
