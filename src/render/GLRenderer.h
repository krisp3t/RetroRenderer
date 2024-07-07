#pragma once
#include <string>
#include <SDL_video.h>

#include "IRenderer.h"

namespace KrisRenderer
{
	class Window;

	class GLRenderer : public IRenderer
	{
	public:
		bool Initialize();
		GLRenderer(Window& window);
		~GLRenderer() override;
		std::string GetName() const override;
		void InitImgui() override;
		void NewFrameImgui() override;
		void RenderImgui() override;
		void DestroyImgui() override;
		void BeginFrame() override;
		void InitializeBuffers() override;
		void ClearBuffers();
		void EndFrame() override;
		void RenderScene() override;
		void OnResize(int width, int height) override;
		SDL_GLContext mGlContext = nullptr;
	private:
		Window& mWindow;
		int mWinWidth;
		int mWinHeight;
	};
}
