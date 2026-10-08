#include "Widgets.hpp"

#include <algorithm>
#include <string>

#include "gui/theme/Theme.hpp"

namespace ui::widgets {
namespace {

void LabelWithHelp(const char* label, const char* help) {
    ImGui::TextUnformatted(label);
    if (help && *help) {
        ImGui::SameLine(0.0f, 6.0f);
        HelpMarker(help);
    }
}

float ControlWidth() {
    return std::clamp(ImGui::GetContentRegionAvail().x * 0.42f, 120.0f, 220.0f);
}

} // namespace

void HelpMarker(const char* text) {
    ImGui::TextDisabled("?");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool Toggle(const char* id, const char* label, bool* value, const char* help) {
    ImGui::PushID(id);
    LabelWithHelp(label, help);
    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - 48.0f));

    const float height = 18.0f;
    const float width = 34.0f;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##toggle", { width, height });
    if (pressed)
        *value = !*value;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 track = ImGui::GetColorU32(*value ? ui::theme::colors::Accent : ui::theme::colors::Border);
    const ImU32 knob = ImGui::GetColorU32(*value ? ui::theme::colors::Canvas : ui::theme::colors::Secondary);
    draw->AddRectFilled(pos, { pos.x + width, pos.y + height }, track, height * 0.5f);
    const float knob_x = *value ? pos.x + width - height * 0.5f : pos.x + height * 0.5f;
    draw->AddCircleFilled({ knob_x, pos.y + height * 0.5f }, height * 0.5f - 3.0f, knob);
    ImGui::PopID();
    return pressed;
}

bool SliderFloat(const char* id, const char* label, float* value, float min, float max,
                 const char* format, const char* help) {
    ImGui::PushID(id);
    LabelWithHelp(label, help);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ControlWidth());
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - ControlWidth() - 12.0f));
    const bool changed = ImGui::SliderFloat("##value", value, min, max, format);
    ImGui::PopID();
    return changed;
}

bool SliderInt(const char* id, const char* label, int* value, int min, int max,
               const char* format, const char* help) {
    ImGui::PushID(id);
    LabelWithHelp(label, help);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ControlWidth());
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - ControlWidth() - 12.0f));
    const bool changed = ImGui::SliderInt("##value", value, min, max, format);
    ImGui::PopID();
    return changed;
}

bool Select(const char* id, const char* label, int* value,
            std::span<const char* const> choices, const char* help) {
    if (choices.empty())
        return false;

    *value = std::clamp(*value, 0, static_cast<int>(choices.size()) - 1);
    ImGui::PushID(id);
    LabelWithHelp(label, help);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ControlWidth());
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - ControlWidth() - 12.0f));

    bool changed = false;
    if (ImGui::BeginCombo("##value", choices[*value])) {
        for (int i = 0; i < static_cast<int>(choices.size()); ++i) {
            if (ImGui::Selectable(choices[i], i == *value)) {
                *value = i;
                changed = true;
            }
            if (i == *value)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return changed;
}

bool Color(const char* id, const char* label, color_t* value) {
    ImGui::PushID(id);
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - 52.0f));
    const bool changed = ImGui::ColorEdit4(
        "##color",
        value->data(),
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf
    );
    ImGui::PopID();
    return changed;
}

void SectionHeader(const char* title, const char* description) {
    ImGui::Spacing();
    ImGui::TextUnformatted(title);
    if (description && *description) {
        ImGui::PushStyleColor(ImGuiCol_Text, ui::theme::colors::Secondary);
        ImGui::TextWrapped("%s", description);
        ImGui::PopStyleColor();
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void StatusPill(const char* label, StatusKind kind) {
    ImVec4 color = ui::theme::colors::Secondary;
    if (kind == StatusKind::Success)
        color = ui::theme::colors::Success;
    else if (kind == StatusKind::Warning)
        color = ui::theme::colors::Warning;
    else if (kind == StatusKind::Danger)
        color = ui::theme::colors::Danger;

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const ImVec2 size{ text_size.x + 14.0f, text_size.y + 6.0f };
    ImGui::Dummy(size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(pos, { pos.x + size.x, pos.y + size.y }, ImGui::GetColorU32(ui::theme::colors::Raised), 3.0f);
    draw->AddRect(pos, { pos.x + size.x, pos.y + size.y }, ImGui::GetColorU32(color), 3.0f);
    draw->AddText({ pos.x + 7.0f, pos.y + 3.0f }, ImGui::GetColorU32(color), label);
}

bool ConfirmButton(const char* id, const char* label, const char* confirmation) {
    ImGui::PushID(id);
    if (ImGui::Button(label))
        ImGui::OpenPopup("Confirm");

    bool confirmed = false;
    if (ImGui::BeginPopupModal("Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", confirmation);
        ImGui::Spacing();
        if (ImGui::Button("Confirm", { 96.0f, 0.0f })) {
            confirmed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 96.0f, 0.0f }))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return confirmed;
}

} // namespace ui::widgets
