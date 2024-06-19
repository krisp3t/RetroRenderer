#include "Application.h"

namespace KrisRenderer
{
	constexpr int DEFAULT_WIN_WIDTH = 1280;
	constexpr int DEFAULT_WIN_HEIGHT = 720;
	std::unique_ptr<Window> Application::sWindow = nullptr;

	Application& Application::Get()
	{
		static Application instance;
		return instance;
	}

	Application::Application()
	{
		sWindow = std::make_unique<Window>(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT);

		/*
		mDisplay = std::make_unique<Display>(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT);
		mDisplay->initialize_window();
		mWindow = std::make_unique<SDL_Window>(mDisplay->get_window());
		mRenderer = std::make_unique<SDL_Renderer>(mDisplay->get_renderer());
		mWindow = mDisplay->get_window();
		mRenderer = mDisplay->get_renderer();
		mGui = std::make_unique<GUI>();
		mGui->initialize_gui(mWindow, mRenderer);
		*/
	}

	void Application::Loop()
	{
		while (sWindow->IsRunning())
		{
			sWindow->HandleEvents();
			sWindow->Update();
			sWindow->Render();
		}
	}
}
