#pragma once
#include <string>

namespace KrisRenderer
{
	class IRenderer
	{
		public:
			virtual ~IRenderer() = default;
			virtual void InitImgui() = 0;
			virtual void NewFrameImgui() = 0;
			virtual void RenderImgui() = 0;
			virtual void DestroyImgui() = 0;
			virtual void BeginFrame() = 0;
			virtual void EndFrame() = 0;
			virtual void RenderScene() = 0;
			virtual void InitializeBuffers() = 0;
			virtual void OnResize(int width, int height) = 0;
			virtual std::string GetName() const = 0;
		// virtual void ClearColorBuffer(uint32_t color) = 0;
		// virtual void RenderColorBuffer() = 0;
	};
}