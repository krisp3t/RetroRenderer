#include "application.h"

namespace MiniRenderer {
	Application& Application::get() {
		static Application sInstance;
		return sInstance;
	}

	Application::Application() {
		display = std::make_unique<Display>();
		display->initialize_window();
	}

	void Application::loop() {
		while (display->is_running()) {
			display->process_input();
			display->update();
			display->render_color_buffer();
		}
	}
}
