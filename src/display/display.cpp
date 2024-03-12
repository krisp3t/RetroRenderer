#include "display.h"

namespace MiniRenderer {
	bool Display::initialize_window() {
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
			printf("Error: %s\n", SDL_GetError());
			return false;
		}

		#ifdef SDL_HINT_IME_SHOW_UI
			SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
		#endif

		SDL_DisplayMode display_mode;
		SDL_GetCurrentDisplayMode(0, &display_mode);
		mWinWidth = display_mode.w;
		mWinHeight = display_mode.h;

		mWindow = SDL_CreateWindow(
			NULL,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			mWinWidth,
			mWinHeight,
			SDL_WINDOW_BORDERLESS
		);

		if (!mWindow)
		{
			fprintf(stderr, "Error creating SDL window.\n");
			return false;
		}

		mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
		if (!mRenderer)
		{
			fprintf(stderr, "Error creating SDL renderer.\n");
			return false;
		}

		SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);
		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		mWindow = SDL_CreateWindow("MiniRenderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
		if (mWindow == nullptr)
		{
			printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
			return false;
		}
		mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
		if (mRenderer == nullptr)
		{
			SDL_Log("Error creating SDL_Renderer!");
			return false;
		}
		mIsRunning = true;
		return true;
	}

	bool Display::is_running() {
		return mIsRunning;
	}

	void Display::process_input() {
		SDL_Event event;
		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			mIsRunning = false;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				mIsRunning = false;
			break;
		}
	}

	void Display::update() {
		//gui->Render();
	}

	void Display::render_color_buffer() {
		//SDL_UpdateTexture(color_buffer_texture, NULL, color_buffer, (int)(mWinWidth * sizeof(uint32_t)));
		//SDL_RenderCopy(mRenderer, color_buffer_texture, NULL, NULL);
		//SDL_RenderPresent(mRenderer);
	}

	void Display::destroy_window(void) {
		//delete[] color_buffer;
		//delete gui;
		SDL_DestroyRenderer(mRenderer);
		SDL_DestroyWindow(mWindow);
		SDL_Quit();
	}

	Display::~Display()
	{
		destroy_window();
	}
}
