#include "DX11Globals.h"

namespace KrisRenderer::DX11Globals {
    Microsoft::WRL::ComPtr< ID3D11Device> sDx11Device = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> sDx11DeviceContext = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory2> sDxgiFactory = nullptr;
}