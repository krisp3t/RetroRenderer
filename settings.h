#pragma once

namespace MiniRenderer
{
	enum LineAlgorithm {
		DDA,
		Bresenham
	};

	struct OpenWindows {
		bool show_camera_window = true;
		bool show_renderer_window = true;
	};

	struct Camera {
		float fov = 60.0f;
		float near = 0.1f;
		float far = 100.0f;
	};

	struct Settings {
		LineAlgorithm line_algo = LineAlgorithm::Bresenham;
		bool draw_wireframe = false;
		OpenWindows open_windows;
		Camera camera;
	};
}