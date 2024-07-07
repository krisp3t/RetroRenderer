#include "GLRenderer.h"
#include "../window/Window.h"

namespace KrisRenderer
{
	GLRenderer::GLRenderer(const Window& window) : mWindow(window)
	{
	}
	GLRenderer::~GLRenderer()
	{
	}
	std::string GLRenderer::GetName() const
	{
		return "OpenGL " + std::to_string(GL_MAJOR_VERSION) + "." + std::to_string(GL_MINOR_VERSION);
	}
	void GLRenderer::BeginFrame()
	{
	}
	void GLRenderer::InitializeBuffers()
	{
	}
	void GLRenderer::ClearBuffers()
	{
	}
	void GLRenderer::EndFrame()
	{
	}
	void GLRenderer::RenderScene()
	{
	}
	void GLRenderer::OnResize(int width, int height)
	{
	}
}