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
			"MiniRenderer",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			mWinWidth,
			mWinHeight,
			SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI
		);

		if (mWindow == nullptr)
		{
			fprintf(stderr, "Error creating SDL window.\n");
			return false;
		}

		mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
		if (mRenderer == nullptr)
		{
			fprintf(stderr, "Error creating SDL renderer.\n");
			return false;
		}

		mColorBuffer = std::make_unique<uint32_t[]>(mWinWidth * mWinHeight);
		mColorBufferTexture = SDL_CreateTexture(
			mRenderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			mWinWidth,
			mWinHeight
		);
		mGui = std::make_unique<GUI>();
		mGui->initialize_gui(mWindow, mRenderer);
		mSettings = std::make_unique<Settings>();
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
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					mWinWidth = event.window.data1;
					mWinHeight = event.window.data2;
					SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window.data2);
				}
				else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
					mIsRunning = false;
				}
				break;
		}
		mGui->process_input(event);
	}

	void Display::update() {
		mGui->update(*mSettings);
	}

	void Display::render_color_buffer() {
		SDL_UpdateTexture(mColorBufferTexture, NULL, mColorBufferTexture, (int)(mWinWidth * sizeof(uint32_t)));
		SDL_RenderCopy(mRenderer, mColorBufferTexture, NULL, NULL);
		SDL_RenderPresent(mRenderer);
	}

	void Display::render() {
		SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
		SDL_RenderClear(mRenderer);
		mGui->render();
		Display::render_color_buffer();
	}

	void Display::destroy_window() {
		SDL_DestroyRenderer(mRenderer);
		SDL_DestroyWindow(mWindow);
		SDL_Quit();
	}

	Display::~Display()
	{
		destroy_window();
	}
}
