#include "GLRenderer.h"
#include "../window/Window.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <SDL_opengl.h>

namespace KrisRenderer
{
	bool GLRenderer::Initialize()
	{
		/*
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_MAJOR_VERSION);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_MINOR_VERSION);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		mGlContext = SDL_GL_CreateContext(mWindow.GetWindow());
		if (mGlContext == nullptr)
		{
			SDL_Log("Unable to create OpenGL context: %s", SDL_GetError());
			return false;
		}
		SDL_GL_MakeCurrent(mWindow.GetWindow(), mGlContext);
		*/
		return true;
	}

	GLRenderer::GLRenderer(const Window& window) : mWindow(window)
	{
		Initialize();
		// SDL_GL_SetSwapInterval(1); // Enable vsync
	}
	GLRenderer::~GLRenderer()
	{
		// SDL_GL_DeleteContext(gl_context);
	}
	std::string GLRenderer::GetName() const
	{
		return "OpenGL " + std::to_string(GL_MAJOR_VERSION) + "." + std::to_string(GL_MINOR_VERSION);
	}
	void GLRenderer::InitImgui()
	{
		/*
		ImGui_ImplSDL2_InitForOpenGL(mWindow.GetWindow(), mGlContext);
		ImGui_ImplOpenGL3_Init(GLSL_VERSION);
		*/
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
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		glViewport(0, 0, 1280, 720);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	void GLRenderer::EndFrame()
	{
		SDL_GL_SwapWindow(mWindow.GetWindow());
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
