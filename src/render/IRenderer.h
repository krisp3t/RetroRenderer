#pragma once

namespace KrisRenderer
{
	class IRenderer
	{
		public:
			virtual ~IRenderer() = default;

			virtual void InitializeBuffers() = 0;
		// virtual void ClearColorBuffer(uint32_t color) = 0;
		// virtual void RenderColorBuffer() = 0;
	};
}