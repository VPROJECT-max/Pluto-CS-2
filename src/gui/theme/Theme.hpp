#pragma once

#include <imgui.h>

namespace ui::theme {

namespace tokens {
inline constexpr float WindowPadding = 12.0f;
inline constexpr float ItemSpacingX = 10.0f;
inline constexpr float ItemSpacingY = 8.0f;
inline constexpr float FramePaddingX = 10.0f;
inline constexpr float FramePaddingY = 6.0f;
inline constexpr float WindowRounding = 7.0f;
inline constexpr float ChildRounding = 6.0f;
inline constexpr float FrameRounding = 4.0f;
inline constexpr float ScrollbarSize = 10.0f;
inline constexpr float NavigationWidth = 156.0f;

static_assert(WindowPadding > 0.0f);
static_assert(FrameRounding >= 0.0f);
static_assert(NavigationWidth > 0.0f);
} // namespace tokens

namespace colors {
inline const ImVec4 Canvas{ 0.0196f, 0.0196f, 0.0196f, 1.0f };       // #050505
inline const ImVec4 Surface{ 0.0392f, 0.0392f, 0.0392f, 1.0f };      // #0A0A0A
inline const ImVec4 Raised{ 0.0667f, 0.0667f, 0.0667f, 1.0f };       // #111111
inline const ImVec4 Hovered{ 0.0902f, 0.0902f, 0.0902f, 1.0f };      // #171717
inline const ImVec4 Border{ 0.1647f, 0.1647f, 0.1647f, 1.0f };       // #2A2A2A
inline const ImVec4 QuietBorder{ 0.1098f, 0.1098f, 0.1098f, 1.0f };  // #1C1C1C
inline const ImVec4 Text{ 0.9529f, 0.9529f, 0.9529f, 1.0f };         // #F3F3F3
inline const ImVec4 Secondary{ 0.6275f, 0.6275f, 0.6275f, 1.0f };    // #A0A0A0
inline const ImVec4 Disabled{ 0.3843f, 0.3843f, 0.3843f, 1.0f };     // #626262
inline const ImVec4 Accent{ 0.9020f, 0.9020f, 0.9020f, 1.0f };       // #E6E6E6
inline const ImVec4 Success{ 0.2980f, 0.7608f, 0.4667f, 1.0f };
inline const ImVec4 Warning{ 0.8863f, 0.6471f, 0.2863f, 1.0f };
inline const ImVec4 Danger{ 0.8863f, 0.3216f, 0.3216f, 1.0f };
} // namespace colors

[[nodiscard]] float ClampDpiScale(float dpi_scale);
void Apply(float dpi_scale = 1.0f, bool reduced_motion = true);

} // namespace ui::theme
