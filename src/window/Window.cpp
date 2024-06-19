#include "Window.h"
#include "../render/DX11Renderer.h"

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

		mWindow = SDL_CreateWindow(
			"KrisRenderer (Mode: Software)",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			mWinWidth,
			mWinHeight,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

		if (mWindow == nullptr)
		{
			SDL_Log("Unable to create window: %s", SDL_GetError());
			return false;
		}
		SDL_Log("AVX support: %s", AVX_SUPPORTED ? "true" : "false");
		mRenderer = std::make_unique<DX11Renderer>(*this);
		mIsRunning = true;
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

	void Window::HandleEvents()
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
				mRenderer->InitializeBuffers();
			}
			else if (event.window.event == SDL_WINDOWEVENT_CLOSE)
			{
				mIsRunning = false;
			}
			break;
		}
		// IMGUI PROCESS INPUT
	}

	void Window::Update()
	{
	}

	void Window::Render()
	{
		mRenderer->Render();
	}
}
