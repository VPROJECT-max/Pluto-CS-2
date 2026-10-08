#pragma once

#include <span>

#include <imgui.h>

#include "core/engine/types/Color.hpp"

namespace ui::widgets {

enum class StatusKind {
    Neutral,
    Success,
    Warning,
    Danger,
};

bool Toggle(const char* id, const char* label, bool* value, const char* help = nullptr);
bool SliderFloat(const char* id, const char* label, float* value, float min, float max,
                 const char* format, const char* help = nullptr);
bool SliderInt(const char* id, const char* label, int* value, int min, int max,
               const char* format, const char* help = nullptr);
bool Select(const char* id, const char* label, int* value,
            std::span<const char* const> choices, const char* help = nullptr);
bool Color(const char* id, const char* label, color_t* value);
void SectionHeader(const char* title, const char* description = nullptr);
void StatusPill(const char* label, StatusKind kind);
bool ConfirmButton(const char* id, const char* label, const char* confirmation);
void HelpMarker(const char* text);

} // namespace ui::widgets
