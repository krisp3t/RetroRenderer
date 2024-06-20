#pragma once
#include <memory>
#include <SDL.h>
#include "IRenderer.h"

namespace KrisRenderer
{
	class Window;

	class SWRenderer : public IRenderer
	{
		public:
			SWRenderer(const Window& window);
			~SWRenderer() override;
			void BeginFrame() override;
			void InitializeBuffers() override;
			void ClearBuffers();
			void EndFrame() override;
			void RenderScene() override;
		private:
			const Window &mWindow;
			SDL_Renderer* mRenderer;
			std::unique_ptr<uint32_t[]> mColorBuffer;
			SDL_Texture* mColorBufferTexture;
	};
}