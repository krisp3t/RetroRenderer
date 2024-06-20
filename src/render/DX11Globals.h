#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace KrisRenderer::DX11Globals {
    extern Microsoft::WRL::ComPtr< ID3D11Device> sDx11Device;
    extern Microsoft::WRL::ComPtr<ID3D11DeviceContext> sDx11DeviceContext;
    extern Microsoft::WRL::ComPtr<IDXGIFactory2> sDxgiFactory;
}
