#include "application.h"

namespace MiniRenderer {
	Application& Application::get() {
		static Application sInstance;
		return sInstance;
	}

	Application::Application() {
		mDisplay = std::make_unique<Display>();
		mDisplay->initialize_window();
	}

	void Application::loop() {
		while (mDisplay->is_running()) {
			mDisplay->process_input();
			mDisplay->update();
			mDisplay->render();
		}
	}
}
