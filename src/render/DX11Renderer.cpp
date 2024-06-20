#include <imgui_impl_sdl2.h>
#include <imgui_impl_dx11.h>

#include "DX11Renderer.h"
#include "DX11Globals.h"
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
		sd.BufferCount = 2;
		sd.OutputWindow = hWnd;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = 0;

		UINT swapCreateFlags = 0u;
#ifndef NDEBUG
		swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			swapCreateFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&sd,
			&pSwapChain,
			&DX11Globals::sDx11Device,
			nullptr,
			&DX11Globals::sDx11DeviceContext
		);
		if (FAILED(hr))
		{
			SDL_Log("Failed to initialize DirectX 11 device and swap chain");
			return;
		}
		SDL_Log("Successfully initialized DX11Renderer");
		ID3D11Resource* pBackBuffer = nullptr; // Gain access to texture subresource in back buffer
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&pBackBuffer));
		DX11Globals::sDx11Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
		pBackBuffer->Release();
	}

	DX11Renderer::~DX11Renderer()
	{
		if (DX11Globals::sDx11DeviceContext) DX11Globals::sDx11DeviceContext->Release();
		if (pSwapChain) pSwapChain->Release();
		if (DX11Globals::sDx11Device) DX11Globals::sDx11Device->Release();
	}


	void DX11Renderer::InitializeBuffers()
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
	}

	void DX11Renderer::ClearBuffers(float red, float green, float blue) noexcept
	{
		const float color[] = { red, green, blue, 1.0f };
		DX11Globals::sDx11DeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
		DX11Globals::sDx11DeviceContext->ClearRenderTargetView(g_pRenderTargetView, color);
	}

	void DX11Renderer::BeginFrame()
	{
		ClearBuffers(1, 0.5, 0);
	}

	void DX11Renderer::RenderScene()
	{
	}

	void DX11Renderer::EndFrame()
	{
		pSwapChain->Present(0u, 0u); // swap buffers
	}


}