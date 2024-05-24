#include "application.h"

namespace MiniRenderer {
	constexpr int DEFAULT_WIN_WIDTH = 1280;
	constexpr int DEFAULT_WIN_HEIGHT = 720;
	constexpr std::string_view DEFAULT_MODEL = "head.obj";

	Application& Application::get() {
		static Application sInstance;
		return sInstance;
	}

	Application::Application() {
		int a = 1280; //TODO: remove
		int b = 720;
		mDisplay = std::make_unique<Display>(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT);
		mDisplay->initialize_window();
		/*
		mWindow = std::make_unique<SDL_Window>(mDisplay->get_window());
		mRenderer = std::make_unique<SDL_Renderer>(mDisplay->get_renderer());
		*/
		mWindow = mDisplay->get_window();
		mRenderer = mDisplay->get_renderer();
		//ResourceManager::get().set_settings(std::make_unique<Settings>(a, b));
		//ResourceManager::get().set_model(std::make_unique<Model>(DEFAULT_MODEL));
		mGui = std::make_unique<GUI>();
		mGui->initialize_gui(mWindow, mRenderer);
	}

	void Application::loop() {
		while (mDisplay->is_running()) {
			mDisplay->process_input();
			mDisplay->update();
			mDisplay->render();
		}
	}
}
