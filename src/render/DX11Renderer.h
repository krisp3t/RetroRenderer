#pragma once
#include <d3d11.h>
#include <dxgi1_3.h>
#include <string>
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
		std::string GetName() const override;
		void InitializeBuffers() override;
		bool InitializeShaders();
		void BeginFrame() override;
		void EndFrame() override;
		bool CompileShader(
			const std::wstring& filename, 
			const std::string& entryPoint, 
			const std::string& profile, 
			ComPtr<ID3DBlob>& shaderBlob) const;
		void RenderScene() override;
		void OnResize(int width, int height) override;
		[[nodiscard]] ComPtr<ID3D11VertexShader> CreateVertexShader(
			const std::wstring& filename,
			ComPtr<ID3DBlob>& vertexShaderBlob) const;
		[[nodiscard]] ComPtr<ID3D11PixelShader> CreatePixelShader(
			const std::wstring& filename) const;
	private:
		ComPtr<IDXGISwapChain1> _SwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> _RenderTargetView = nullptr;
		bool CreateSwapchainResources();
		void DestroySwapchainResources();
		int mWinWidth;
		int mWinHeight;
		ComPtr<ID3D11VertexShader> _VertexShader = nullptr;
		ComPtr<ID3D11PixelShader> _PixelShader = nullptr;
		ComPtr<ID3D11InputLayout> _InputLayout = nullptr;
		ComPtr<ID3D11Buffer> _VertexBuffer = nullptr;
	};
}