#pragma once
#include <glad/glad.h>
#include "../../Scene/Camera.h"
#include "../../Scene/Scene.h"
#include "../IRenderer.h"

namespace RetroRenderer
{
	class GLRenderer : public IRenderer
	{
	public:
		GLRenderer() = default;
		~GLRenderer() = default;

		bool Init(int w, int h);
		void Destroy();
		void SetActiveCamera(const Camera& camera);
		void DrawTriangularMesh(const Model* model);
		void Render();
		GLuint GetRenderTarget();
	private:
		GLuint m_FrameBuffer = 0;
		Camera* p_Camera = nullptr;
	};
}