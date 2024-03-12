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
		void clear_color_buffer(void);
		void render_color_buffer(void);
		void draw_pixel(int x, int y, uint32_t color);
		void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
		bool is_running();
	private:
		SDL_Window* mWindow = nullptr;
		SDL_Renderer* mRenderer = nullptr;
		uint32_t* mColorBuffer = nullptr;
		SDL_Texture* mColorBufferTexture = nullptr;
		std::unique_ptr<GUI> mGui;
		std::unique_ptr<Settings> mSettings;
		ImGuiIO* mIo;
		int mWinWidth = 800;
		int mWinHeight = 600;
		bool mIsRunning = true;
	};
}
