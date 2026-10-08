// imgui_custom.cpp
// Adapted from the new UI for the old project (Direct3D/Win32 backend, no SFML).
// StringH and ColorH utility functions are inlined here to avoid external dependencies.

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_custom.hpp"

#include <Windows.h>
#include <string>

// ---------------------------------------------------------------------------
// Inlined VK -> string helper (replaces StringH::vkToString)
// ---------------------------------------------------------------------------
static std::string VkToStr(int vk) {
#define caseStr(x) case x: return std::string(#x + 3)
	char c[2] = { 0 };
	if (vk >= '0' && vk <= '9') { c[0] = (char)vk; return std::string(c); }
	if (vk >= 'A' && vk <= 'Z') { c[0] = (char)vk; return std::string(c); }
	switch (vk) {
	case VK_LBUTTON: return "LMB";
	case VK_RBUTTON: return "RMB";
	case VK_MBUTTON: return "MMB";
	case VK_XBUTTON1: return "MB4";
	case VK_XBUTTON2: return "MB5";
		caseStr(VK_BACK);
		caseStr(VK_TAB);
		caseStr(VK_RETURN);
		caseStr(VK_SHIFT);
		caseStr(VK_CONTROL);
		caseStr(VK_MENU);
		caseStr(VK_PAUSE);
		caseStr(VK_CAPITAL);
		caseStr(VK_ESCAPE);
		caseStr(VK_SPACE);
		caseStr(VK_PRIOR);
		caseStr(VK_NEXT);
		caseStr(VK_END);
		caseStr(VK_HOME);
		caseStr(VK_LEFT);
		caseStr(VK_UP);
		caseStr(VK_RIGHT);
		caseStr(VK_DOWN);
		caseStr(VK_INSERT);
		caseStr(VK_DELETE);
		caseStr(VK_LWIN);
		caseStr(VK_RWIN);
		caseStr(VK_NUMPAD0);
		caseStr(VK_NUMPAD1);
		caseStr(VK_NUMPAD2);
		caseStr(VK_NUMPAD3);
		caseStr(VK_NUMPAD4);
		caseStr(VK_NUMPAD5);
		caseStr(VK_NUMPAD6);
		caseStr(VK_NUMPAD7);
		caseStr(VK_NUMPAD8);
		caseStr(VK_NUMPAD9);
	case VK_MULTIPLY: return "*";
		caseStr(VK_ADD);
		caseStr(VK_SUBTRACT);
	case VK_DIVIDE: return "/";
		caseStr(VK_F1);  caseStr(VK_F2);  caseStr(VK_F3);  caseStr(VK_F4);
		caseStr(VK_F5);  caseStr(VK_F6);  caseStr(VK_F7);  caseStr(VK_F8);
		caseStr(VK_F9);  caseStr(VK_F10); caseStr(VK_F11); caseStr(VK_F12);
		caseStr(VK_NUMLOCK);
		caseStr(VK_SCROLL);
		caseStr(VK_LSHIFT);
		caseStr(VK_RSHIFT);
		caseStr(VK_LCONTROL);
		caseStr(VK_RCONTROL);
	case VK_LMENU: return "LALT";
	case VK_RMENU: return "RALT";
	case VK_OEM_PLUS:   return "+";
	case VK_OEM_COMMA:  return ",";
	case VK_OEM_MINUS:  return "-";
	case VK_OEM_PERIOD: return ".";
	case 0: return "None";
	}
	c[0] = (char)vk;
	return std::string(c);
#undef caseStr
}

// ---------------------------------------------------------------------------
// ImGui::Checkbox_ / Checkbox2
// ---------------------------------------------------------------------------
bool ImGui::Checkbox_(const char* label, bool* v) {
	if (STYLE == 0) return Checkbox2(label, v);
	return Checkbox(label, v); // fallback
}

bool ImGui::Checkbox2(const char* label, bool* v) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems) return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const float square_sz = GetFrameHeight();
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
	{
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return false;
	}

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}

	const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
	RenderNavHighlight(total_bb, id);
	RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
	ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
	bool mixed_value = (g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0;
	if (mixed_value)
	{
		ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
		window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
	}
	else if (*v)
	{
		const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
		RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
	}

	ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
	if (g.LogEnabled)
		LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
	if (label_size.x > 0.0f)
		RenderText(label_pos, label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}

// ---------------------------------------------------------------------------
// Slider wrappers
// ---------------------------------------------------------------------------
bool ImGui::SliderFloat_2(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
	return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}
bool ImGui::SliderFloat_(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
	if (STYLE == 0) return SliderFloat_2(label, v, v_min, v_max, format, flags);
	return SliderFloat(label, v, v_min, v_max, format, flags);
}

bool ImGui::SliderInt_2(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
	return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}
bool ImGui::SliderInt_(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
	if (STYLE == 0) return SliderInt_2(label, v, v_min, v_max, format, flags);
	return SliderInt(label, v, v_min, v_max, format, flags);
}

// ---------------------------------------------------------------------------
// ImGuiTextFilter2::Draw2  (search box helper)
// ---------------------------------------------------------------------------
bool ImGuiTextFilter2::Draw2(const char* label, float width) {
	if (width != 0.0f)
		ImGui::SetNextItemWidth(width);

	std::string id = std::string("##Input_") += label;
	bool value_changed = ImGui::InputTextWithHint(id.c_str(), label, InputBuf, IM_ARRAYSIZE(InputBuf));
	if (value_changed)
		Build();
	return value_changed;
}

// ---------------------------------------------------------------------------
// ImGui::Hotkey  — polls currently held key while the button is hovered
// ---------------------------------------------------------------------------
bool ImGui::Hotkey(const char* label, int& key, float samelineOffset, const ImVec2& size) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems) return false;

	TextUnformatted(label);
	SameLine(samelineOffset);

	Button(key == 0 ? "..." : VkToStr(key).c_str(), size);
	if (IsItemHovered()) {
		for (auto i = VK_MBUTTON; i <= VK_PACKET; i++) {
			if (i == VK_ESCAPE) continue;
			if (GetAsyncKeyState(i) & 0x8000) {
				key = i;
			}
		}
	}

	return true;
}
