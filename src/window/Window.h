#pragma once
#include <SDL.h>
#include "../defines.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

namespace KrisRenderer
{
	class Window
	{
	public:
		Window();
		Window(int width, int height);
		~Window();
		bool InitializeWindow();

		SDL_Window* get_window() const;
		SDL_Renderer* get_renderer() const;
		void process_input();
	private:
		SDL_Window* mWindow = nullptr;
		SDL_Renderer* mRenderer = nullptr;
		int mWinWidth;
		int mWinHeight;
		bool mIsRunning = true;
	};
}