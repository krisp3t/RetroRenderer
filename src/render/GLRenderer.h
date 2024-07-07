#pragma once
#define GL_MAJOR_VERSION 3
#define GL_MINOR_VERSION 3
#include <string>
#include "IRenderer.h"

namespace KrisRenderer
{
	class Window;

	class GLRenderer : public IRenderer
	{
	public:
		GLRenderer(const Window& window);
		~GLRenderer() override;
		std::string GetName() const override;
		void BeginFrame() override;
		void InitializeBuffers() override;
		void ClearBuffers();
		void EndFrame() override;
		void RenderScene() override;
		void OnResize(int width, int height) override;
	private:
		const Window& mWindow;
	};
}