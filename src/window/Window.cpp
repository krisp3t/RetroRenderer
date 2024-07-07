#include "Window.h"
#include "Gui.h"
#include "../render/DX11Renderer.h"
#include "../render/GLRenderer.h"
#include "../render/SWRenderer.h"
#include <imgui_impl_opengl3.h>

#include "imgui_impl_sdl2.h"


namespace KrisRenderer
{
	Window::Window()
	{
		mWinWidth = 1280;
		mWinHeight = 720;
		InitializeWindow();
	}

	Window::Window(const int width, const int height)
	{
		assert(width > 0 && height > 0);
		mWinWidth = width;
		mWinHeight = height;
		InitializeWindow();
	}

	Window::~Window()
	{
		SDL_DestroyWindow(mWindow);
	}

	bool Window::InitializeWindow()
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
			return false;
		}
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_MAJOR_VERSION);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_MINOR_VERSION);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		bool isOpengl = true; // TODO: replace with enum
		SDL_WindowFlags windowFlags = static_cast<SDL_WindowFlags>(
			SDL_WINDOW_RESIZABLE | 
			SDL_WINDOW_ALLOW_HIGHDPI |
			(isOpengl ? SDL_WINDOW_OPENGL : 0)
		);

		mWindow = SDL_CreateWindow(
			nullptr,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			mWinWidth,
			mWinHeight,
			windowFlags
		);
		if (mWindow == nullptr)
		{
			SDL_Log("Unable to create window: %s", SDL_GetError());
			return false;
		}
		auto mGlContext = SDL_GL_CreateContext(GetWindow());
		if (mGlContext == nullptr)
		{
			SDL_Log("Unable to create OpenGL context: %s", SDL_GetError());
			return false;
		}
		SDL_GL_MakeCurrent(GetWindow(), mGlContext);

		SDL_Log("AVX support: %s", AVX_SUPPORTED ? "true" : "false");

		mRenderer = std::make_unique<GLRenderer>(*this);
		const std::string title = "KrisRenderer (Mode: " + mRenderer->GetName() + ")";
		SDL_SetWindowTitle(mWindow, title.c_str());
		mIsRunning = true;


		




		mGui = std::make_unique<Gui>(*mRenderer);
		ImGui_ImplSDL2_InitForOpenGL(GetWindow(), mGlContext);
		ImGui_ImplOpenGL3_Init(GLSL_VERSION);

		assert(mWindow != nullptr);
		assert(mRenderer != nullptr);
		assert(mGui != nullptr);
		return true;
	}

	std::pair<int, int> Window::GetSize() const
	{
		return std::pair<uint32_t, uint32_t>{mWinWidth, mWinHeight};
	}

	SDL_Window* Window::GetWindow() const
	{
		return mWindow;
	}

	bool Window::IsRunning() const
	{
		return mIsRunning;
	}

	void Window::HandleInput()
	{
		SDL_Event event;
		SDL_PollEvent(&event);
		switch (event.type)
		{
		case SDL_QUIT:
			mIsRunning = false;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				mIsRunning = false;
			}
			break;
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				mWinWidth = event.window.data1;
				mWinHeight = event.window.data2;
				SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window.data2);
				mRenderer->OnResize(mWinWidth, mWinHeight);
			}
			else if (event.window.event == SDL_WINDOWEVENT_CLOSE)
			{
				mIsRunning = false;
			}
			break;
		}
		mGui->ProcessInput(event);
	}

	void Window::Update()
	{
		mRenderer->BeginFrame();
		mGui->BeginFrame();
	}

	void Window::Render()
	{
		mRenderer->RenderScene();
		mGui->RenderScene();
		mGui->EndFrame();
		mRenderer->EndFrame();

	}
}
