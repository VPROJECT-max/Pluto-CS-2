#pragma once

#include <imgui.h>

struct ID3D11Device;

class EspPreview {
public:
    [[nodiscard]] static bool Initialize(ID3D11Device* device);
    static void Shutdown();
    static void Render(ImVec2 size);
};
