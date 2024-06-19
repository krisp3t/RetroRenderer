#include "Window.h"

namespace KrisRenderer
{
	Window::Window()
	{
		mWinWidth = 1280;
		mWinHeight = 720;
		InitializeWindow();
	}

	Window::Window(int width, int height)
	{
		mWinWidth = width;
		mWinHeight = height;
		InitializeWindow();
	}

	Window::~Window()
	{
		SDL_DestroyRenderer(mRenderer);
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

		mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);
		if (mRenderer == nullptr)
		{
			SDL_Log("Unable to create renderer: %s", SDL_GetError());
			return false;
		}

		return true;
	}


}