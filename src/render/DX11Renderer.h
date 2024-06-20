#pragma once
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include "IRenderer.h"

namespace KrisRenderer
{

	class Window;
	class DX11Renderer : public IRenderer
	{
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
	public:
		bool Initialize(HWND h);
		DX11Renderer(const Window& window);
		DX11Renderer(const DX11Renderer&) = delete;
		DX11Renderer& operator=(const DX11Renderer&) = delete;
		~DX11Renderer() override;
		void InitializeBuffers() override;
		void BeginFrame() override;
		void EndFrame() override;
		void RenderScene() override;
		void OnResize(int width, int height) override;
	private:
		ComPtr<IDXGISwapChain1> _SwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> _RenderTargetView = nullptr;
		bool CreateSwapchainResources();
		void DestroySwapchainResources();
		int mWinWidth;
		int mWinHeight;
	};
}