#pragma once

#include <iostream>
#include "display/display.h"

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
		std::unique_ptr<Display> display;
	};
}