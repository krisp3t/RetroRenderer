#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <memory>
#include <wrl/client.h>

namespace KrisRenderer::DX11Globals {
    class DX11DebugCleanup {
    public:
        DX11DebugCleanup(Microsoft::WRL::ComPtr<ID3D11Debug>& debug) : debugLayer(debug) {}

        ~DX11DebugCleanup() {
            if (debugLayer) {
                debugLayer->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
            }
        }

    private:
        Microsoft::WRL::ComPtr<ID3D11Debug> debugLayer;
    };

    extern Microsoft::WRL::ComPtr<ID3D11Device> sDx11Device;
    extern Microsoft::WRL::ComPtr<ID3D11DeviceContext> sDx11DeviceContext;
    extern Microsoft::WRL::ComPtr<IDXGIFactory2> sDxgiFactory;
    extern Microsoft::WRL::ComPtr<ID3D11Debug> sDx11Debug;
    extern std::unique_ptr<DX11DebugCleanup> sDx11DebugCleanup;
}
