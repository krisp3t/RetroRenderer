#pragma once
#include <SDL.h>
#include <iostream>
#include "GUI.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

namespace MiniRenderer {
	class Display
	{
	public:
		Display() = default;
		~Display();
		bool initialize_window(void);
		void destroy_window(void);
		void process_input(void);
		void update(void);
		void render(void);
		void render_color_buffer(void);
		bool is_running();
	private:
		SDL_Window* mWindow = nullptr;
		SDL_Renderer* mRenderer = nullptr;
		std::unique_ptr<uint32_t[]> mColorBuffer;
		SDL_Texture* mColorBufferTexture = nullptr;
		std::unique_ptr<GUI> mGui;
		std::unique_ptr<Settings> mSettings;
		int mWinWidth = 800;
		int mWinHeight = 600;
		bool mIsRunning = true;
	};
}
