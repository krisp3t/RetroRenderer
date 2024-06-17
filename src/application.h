#pragma once
#include <memory>

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
		// std::unique_ptr<Window> mWindow;
		// std::unique_ptr<Renderer> mRenderer;
	};
}
