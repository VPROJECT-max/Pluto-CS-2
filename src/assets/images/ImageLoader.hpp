#pragma once
#include <d3d11.h>
#include <string>

class ImageLoader {
public:
    static bool LoadTextureFromMemory(ID3D11Device* device, const unsigned char* image_data, int image_data_len, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
};
