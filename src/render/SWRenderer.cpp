#include "SWRenderer.h"
#include "../window/Window.h"

namespace KrisRenderer
{

	SWRenderer::SWRenderer(const Window &window)
		: mWindow(window)
	{
		mRenderer = SDL_CreateRenderer(window.GetWindow(), -1, SDL_RENDERER_SOFTWARE);
		if (mRenderer == nullptr)
		{
			SDL_Log("Unable to create renderer: %s", SDL_GetError());
		}
		InitializeBuffers();

		SDL_Log("Successfully initiaized SWRenderer");
	}

	SWRenderer::~SWRenderer()
	{
		SDL_DestroyTexture(mColorBufferTexture);
		SDL_DestroyRenderer(mRenderer);
	}

	void SWRenderer::InitializeBuffers()
	{
		auto size = mWindow.GetSize();
		uint32_t bufferSize = size.first * size.second;

		mColorBuffer = std::make_unique<uint32_t[]>(bufferSize);
		mColorBufferTexture = SDL_CreateTexture(
			mRenderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			size.first,
			size.second
		);
	}
	void SWRenderer::ClearBuffers()
	{
		memset(mColorBuffer.get(), 0x00000000, mWindow.GetSize().first * mWindow.GetSize().second * sizeof(uint32_t));
	}
	void SWRenderer::BeginFrame()
	{
		ClearBuffers();
	}
	void SWRenderer::RenderScene()
	{
	}
	void SWRenderer::EndFrame()
	{
		SDL_UpdateTexture(mColorBufferTexture, nullptr, mColorBuffer.get(), mWindow.GetSize().first * sizeof(uint32_t));
		SDL_RenderCopy(mRenderer, mColorBufferTexture, nullptr, nullptr);
		SDL_RenderPresent(mRenderer);
	}
}