#include "Window.h"

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
		mRenderer = std::make_unique<SWRenderer>(*this);
		/*
		mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);
		if (mRenderer == nullptr)
		{
			SDL_Log("Unable to create renderer: %s", SDL_GetError());
			return false;
		}
		*/
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

}
