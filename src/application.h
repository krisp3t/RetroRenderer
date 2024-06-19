#pragma once
#include <memory>
#include "window/Window.h"

// Meyers Singleton
namespace KrisRenderer
{
	class Application
	{
	public:
		static Application& Get();
		Application(const Application&) = delete;
		void operator=(const Application&) = delete;
		void Loop();
	private:
		Application();
		static std::unique_ptr<Window> sWindow;
		// std::unique_ptr<Renderer> mRenderer;
	};
}
