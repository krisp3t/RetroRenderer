#pragma once
#include <memory>
#include <SDL.h>
#include "../window/Window.h"

namespace KrisRenderer
{
	class Window;
	class SWRenderer : public IRenderer
	{
		public:
			SWRenderer(const Window& window);
			~SWRenderer() override;
			void InitializeBuffers() override;
			void ClearBuffers();
			void Render();
		private:
			const Window &mWindow;
			SDL_Renderer* mRenderer;
			std::unique_ptr<uint32_t[]> mColorBuffer;
			SDL_Texture* mColorBufferTexture;
	};
}