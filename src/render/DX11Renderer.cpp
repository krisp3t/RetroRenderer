#include <imgui_impl_sdl2.h>
#include <imgui_impl_dx11.h>

#include "DX11Renderer.h"
#include "DX11Globals.h"
#include "SDL_syswm.h"
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "../Application.h"
#include "../window/Window.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

namespace KrisRenderer
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	bool DX11Renderer::Initialize(HWND h)
	{
		// Create DXGI factory
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&DX11Globals::sDxgiFactory))))
		{
			SDL_Log("DXGI: Failed to create factory");
			return false;
		}
		constexpr D3D_FEATURE_LEVEL DeviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
		if (DX11Globals::sDx11Device != nullptr)
		{
			SDL_Log("DX11: Device already initialized");
		}
		if (DX11Globals::sDx11DeviceContext != nullptr)
		{
			SDL_Log("DX11: Device context already initialized");
		}
		UINT SwapCreateFlags = 0u;
#ifndef NDEBUG
		SwapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		if ((DX11Globals::sDx11Device == nullptr) && (DX11Globals::sDx11DeviceContext == nullptr))
		{
			// Create device and device context
			if (FAILED(D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				SwapCreateFlags,
				&DeviceFeatureLevel,
				1,
				D3D11_SDK_VERSION,
				&DX11Globals::sDx11Device,
				nullptr,
				&DX11Globals::sDx11DeviceContext)))
			{
				SDL_Log("D3D11: Failed to create device and device Context");
				return false;
			}
		}
		DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
		swapChainDescriptor.Width = mWinWidth;
		swapChainDescriptor.Height = mWinHeight;
		swapChainDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDescriptor.SampleDesc.Count = 1;
		swapChainDescriptor.SampleDesc.Quality = 0;
		swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescriptor.BufferCount = 2;
		swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDescriptor.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
		swapChainDescriptor.Flags = {};
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
		swapChainFullscreenDescriptor.Windowed = true;
		// Create swapchain
		if (FAILED(DX11Globals::sDxgiFactory->CreateSwapChainForHwnd(
			DX11Globals::sDx11Device.Get(),
			h,
			&swapChainDescriptor,
			&swapChainFullscreenDescriptor,
			nullptr,
			&_SwapChain)))
		{
			SDL_Log("DXGI: Failed to create swap chain");
			return false;
		}
		// Create render target view
		if (!CreateSwapchainResources())
		{
			return false;
		}
		
		return true;
	}
	DX11Renderer::DX11Renderer(const Window& window)
	{
		auto size = window.GetSize();
		mWinWidth = size.first;
		mWinHeight = size.second;
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window.GetWindow(), &wmInfo);
		HWND hWnd = (HWND)wmInfo.info.win.window;
		if (Initialize(hWnd))
		{
			SDL_Log("Successfully initialized DX11Renderer");
		}
		else
		{
			SDL_Log("Failed to initialize DX11Renderer");
		}
	}

	DX11Renderer::~DX11Renderer()
	{
		/*
		DX11Globals::sDx11DeviceContext->Flush();
		DestroySwapchainResources();
		_SwapChain.Reset();
	
		_dxgiFactory.Reset();
		_deviceContext.Reset();
		_device.Reset();
		*/
	}

	void DX11Renderer::InitializeBuffers()
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
	}

	void DX11Renderer::BeginFrame()
	{
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<float>(mWinWidth);
		viewport.Height = static_cast<float>(mWinHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		constexpr float clearColor[] = { 0.5f, 0.1f, 1.0f, 1.0f };
		DX11Globals::sDx11DeviceContext->ClearRenderTargetView(_RenderTargetView.Get(), clearColor);
		DX11Globals::sDx11DeviceContext->RSSetViewports(1, &viewport);
		DX11Globals::sDx11DeviceContext->OMSetRenderTargets(1, _RenderTargetView.GetAddressOf(), nullptr);
	}

	void DX11Renderer::RenderScene()
	{
	}

	void DX11Renderer::OnResize(int width, int height)
	{
		mWinWidth = width;
		mWinHeight = height;
		// Release the existing render target view
		DX11Globals::sDx11DeviceContext->Flush();
		DestroySwapchainResources();

		// Resize the swap chain
		if (FAILED(_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
		{
			SDL_Log("D3D11: Failed to resize swap chain buffers");
			return;
		}

		// Recreate the render target view
		if (!CreateSwapchainResources())
		{
			SDL_Log("D3D11: Failed to create render target view after resizing swap chain");
			return;
		}

		SDL_Log("D3D11: Resized swap chain to %d x %d", width, height);
	}


	bool DX11Renderer::CreateSwapchainResources()
	{
		ComPtr<ID3D11Texture2D> backBuffer = nullptr;
		if (FAILED(_SwapChain->GetBuffer(
			0,
			IID_PPV_ARGS(&backBuffer))))
		{
			SDL_Log("D3D11: Failed to get back buffer from the swapchain");
			return false;
		}
		if (FAILED(DX11Globals::sDx11Device->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			&_RenderTargetView)))
		{
			SDL_Log("D3D11: Failed to create rendertarget view from back buffer");
			return false;
		}

		return true;
	}

	void DX11Renderer::DestroySwapchainResources()
	{
		_RenderTargetView.Reset();
	}

	void DX11Renderer::EndFrame()
	{
		_SwapChain->Present(0u, 0u); // swap buffers
	}

    bool DX11Renderer::CompileShader(
        const std::wstring& filename,
        const std::string& entryPoint,
        const std::string& profile,
        ComPtr<ID3DBlob>& shaderBlob) const
    {
    #ifndef NDEBUG
        constexpr UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
    #else
        constexpr UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
    #endif
        ComPtr<ID3DBlob> tempShaderBlob = nullptr; // we load the compiled shader into this blob
        ComPtr<ID3DBlob> errorBlob = nullptr;
        if (FAILED(D3DCompileFromFile(
            filename.data(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.data(),
            profile.data(),
            compileFlags,
            0,
            &tempShaderBlob,
            &errorBlob
        )))
        {
            SDL_Log("D3D11: Failed to compile shader: %s", filename.c_str());
            if (errorBlob != nullptr)
            {
                SDL_Log("D3D11: Shader compilation error: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            return false;
        }
        shaderBlob = std::move(tempShaderBlob);
        return true;
    }

	ComPtr<ID3D11VertexShader> DX11Renderer::CreateVertexShader(
		const std::wstring& filename, 
		ComPtr<ID3DBlob>& vertexShaderBlob) const
	{
		if (!CompileShader(filename, "Main", "vs_5_0", vertexShaderBlob))
		{
			return nullptr;
		}
		ComPtr<ID3D11VertexShader> vertexShader = nullptr;
		if (FAILED(DX11Globals::sDx11Device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),
			vertexShaderBlob->GetBufferSize(),
			nullptr,
			&vertexShader)))
		{
			SDL_Log("D3D11: Failed to create vertex shader from compiled shader blob");
			return nullptr;
		}
		return vertexShader;
	}

	ComPtr<ID3D11PixelShader> DX11Renderer::CreatePixelShader(
		const std::wstring& filename) const
	{
		ComPtr<ID3DBlob> pixelShaderBlob = nullptr;
		if (!CompileShader(filename, "Main", "ps_5_0", pixelShaderBlob))
		{
			return nullptr;
		}
		ComPtr<ID3D11PixelShader> pixelShader = nullptr;
		if (FAILED(DX11Globals::sDx11Device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),
			pixelShaderBlob->GetBufferSize(),
			nullptr,
			&pixelShader)))
		{
			SDL_Log("D3D11: Failed to create pixel shader from compiled shader blob");
			return nullptr;
		}
		return pixelShader;
	}

}