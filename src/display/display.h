#pragma once
#include <SDL.h>
#include <iostream>
#include <vector>
#include "GUI.h"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "../model.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

namespace MiniRenderer {
	class Display
	{
	public:
		Display() = default;
		~Display();
		void initialize_buffers(void);
		bool initialize_window(void);
		void destroy_window(void);
		void process_input(void);
		void update(void);
		void draw_grid();
		void render(void);
		void clear_color_buffer(void);
		void draw_pixel(int x, int y, uint32_t color);
		void draw_DDA(int x0, int y0, int x1, int y1, uint32_t color);
		void draw_bresenham(int x0, int y0, int x1, int y1, uint32_t color);
		void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
		void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
		void draw_rect(int x, int y, int width, int height, uint32_t color);
		glm::vec2 project(glm::vec3 point);
		bool is_running();
	private:
		SDL_Window* mWindow = nullptr;
		SDL_Renderer* mRenderer = nullptr;
		uint32_t* mColorBuffer = nullptr;
		SDL_Texture* mColorBufferTexture = nullptr;
		std::unique_ptr<GUI> mGui;
		std::unique_ptr<Settings> mSettings;
		ImGuiIO* mIo;
		int mWinWidth = 800;
		int mWinHeight = 600;
		bool mIsRunning = true;

		glm::vec3 mCameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mCameraRot = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mCubeRot = glm::vec3(0.0f, 0.0f, 0.0f);
		Model* mModel = nullptr;
		std::vector<glm::vec3> mCubeVertices;
		float fov_factor = 640;

	};
}
