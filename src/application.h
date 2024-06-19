#pragma once
#include <memory>


// Meyers Singleton
namespace KrisRenderer
{
	class Window;

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
