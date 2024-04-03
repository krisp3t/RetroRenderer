#pragma once
#include <string>

namespace MiniRenderer
{
	enum LineAlgorithm {
		DDA,
		Bresenham,
		Wu
	};

	enum TriangleAlgo {
		Wireframe,
		Flat,
		Gouraud
	};

	struct OpenWindows {
		bool show_camera_window = true;
		bool show_renderer_window = true;
	};

	struct Camera {
		float fov = 60.0f;
		float near = 0.1f;
		float far = 100.0f;
		float position[3] = { 0.0f, 0.0f, 3.0f };
		float rotation[3] = { 0.0f, 0.0f, 3.0f };
	};

	struct Settings {
		Settings(int& width, int& height) : winWidth(width), winHeight(height) {}

		LineAlgorithm line_algo = LineAlgorithm::Bresenham;
		TriangleAlgo triangle_algo = TriangleAlgo::Wireframe;
		OpenWindows open_windows;
		Camera camera;
		int& winWidth;
		int& winHeight;
		std::string filename;
		std::string filepath;

		float fg_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		bool backface_culling = true;
	};
}