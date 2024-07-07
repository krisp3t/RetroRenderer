#include "GLRenderer.h"
#include "../window/Window.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>>

namespace KrisRenderer
{
	GLRenderer::GLRenderer(const Window& window) : mWindow(window)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_MAJOR_VERSION);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,GL_MINOR_VERSION);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GLContext gl_context = SDL_GL_CreateContext(mWindow.GetWindow());
		SDL_GL_MakeCurrent(mWindow.GetWindow(), gl_context);
		SDL_GL_SetSwapInterval(1); // Enable vsync

		// TODO: error checking

		mGlContext = gl_context;
	}
	GLRenderer::~GLRenderer()
	{
	}
	std::string GLRenderer::GetName() const
	{
		return "OpenGL " + std::to_string(GL_MAJOR_VERSION) + "." + std::to_string(GL_MINOR_VERSION);
	}
	void GLRenderer::InitImgui()
	{
		ImGui_ImplSDL2_InitForOpenGL(mWindow.GetWindow(), mGlContext);
		ImGui_ImplOpenGL3_Init(GLSL_VERSION);
	}

	void GLRenderer::NewFrameImgui()
	{
		ImGui_ImplOpenGL3_NewFrame();
	}

	void GLRenderer::RenderImgui()
	{
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void GLRenderer::DestroyImgui()
	{
		ImGui_ImplOpenGL3_Shutdown();
	}

	void GLRenderer::BeginFrame()
	{
	}
	void GLRenderer::EndFrame()
	{
	}
	void GLRenderer::RenderScene()
	{
	}
	void GLRenderer::InitializeBuffers()
	{
	}
	void GLRenderer::ClearBuffers()
	{
	}

	void GLRenderer::OnResize(int width, int height)
	{
	}
}
