#pragma once
#include <memory>
#include <d3d11.h>
#include <SDL_syswm.h>
#include "IRenderer.h"

namespace KrisRenderer
{
	class Window;

	class DX11Renderer : public IRenderer
	{
	public:
		DX11Renderer(const Window& window);
		DX11Renderer(const DX11Renderer&) = delete;
		DX11Renderer& operator=(const DX11Renderer&) = delete;
		~DX11Renderer() override;
		void InitializeBuffers() override;
		void ClearBuffers();
		void Render() override;
	private:
		IDXGISwapChain* pSwapChain;
	};
}