#pragma once

#include <iostream>
#include <memory>
#include <SDL.h>
#include "display/display.h"
#include "resourcemanager.h"

namespace MiniRenderer {
	class Application 
	{
	public:
		Application();
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		static Application& get();
		void loop();
	private:
		static Application* sInstance;
		std::unique_ptr<Display> mDisplay;
		SDL_Window* mWindow;
		SDL_Renderer* mRenderer;
		std::unique_ptr<GUI> mGui;
	};
}