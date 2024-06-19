#include "DX11Renderer.h"
#include "../Application.h"
#include "../window/Window.h"

namespace KrisRenderer
{
	DX11Renderer::DX11Renderer(const Window& window)
	{
		auto size = window.GetSize();
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window.GetWindow(), &wmInfo);
		HWND hWnd = (HWND)wmInfo.info.win.window;

		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferDesc.Width = size.first;
		sd.BufferDesc.Height = size.second;
		sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 0;
		sd.BufferDesc.RefreshRate.Denominator = 0;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.OutputWindow = hWnd;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		UINT swapCreateFlags = 0u;
#ifndef NDEBUG
		swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&sd,
			&pSwapChain,
			&Application::sDx11Device,
			nullptr,
			&Application::sDx11DeviceContext
		);
	}

	DX11Renderer::~DX11Renderer()
	{
		if (Application::sDx11DeviceContext) Application::sDx11DeviceContext->Release();
		if (pSwapChain) pSwapChain->Release();
		if (Application::sDx11Device) Application::sDx11Device->Release();
	}

	void DX11Renderer::InitializeBuffers()
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
	}

	void DX11Renderer::ClearBuffers()
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
	}

	void DX11Renderer::Render()
	{
		ID3D11Texture2D* pBackBuffer = nullptr;

	}
}