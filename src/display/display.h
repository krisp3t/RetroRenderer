#pragma once
#include <SDL.h>
#include <iostream>
#include <vector>
#include <functional>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "../model.h"
#include "utility.h"
#include "GUI.h"

#ifdef __AVX__
#include <immintrin.h>
#define AVX_SUPPORTED true
#else
#define AVX_SUPPORTED false
#endif




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
		void fill_flat_bottom_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 mid);
		void fill_flat_top_triangle(glm::vec2 v1, glm::vec2 mid, glm::vec2 v2);
		void draw_model_flat(std::array<glm::vec3, 3>& vertices);
		void draw_model_wireframe(std::array<glm::vec3, 3>& vertices);
		void apply_transformations(std::array<glm::vec3, 3>& vertices);
		bool backside_cull(std::array<glm::vec3, 3>& vertices);
		void draw_model(void);
		void render(void);
		void clear_color_buffer(void);
		void draw_pixel(int x, int y, uint32_t color);
		void draw_DDA(int x0, int y0, int x1, int y1, uint32_t color);
		void draw_bresenham(int x0, int y0, int x1, int y1, uint32_t color);
		void draw_line(glm::vec2 p0, glm::vec2 p1, uint32_t color);
		void draw_wu(int x0, int y0, int x1, int y1, uint32_t color);
		void plot(int x, int y, float intensity, uint32_t color);
		float fpart(float x);
		float rfpart(float x);
		int ipart(float x);
		uint32_t blendColors(uint32_t color, uint8_t alpha);
		void draw_rect(int x, int y, int width, int height, uint32_t color);
		glm::vec2 project(glm::vec3 point);
		bool is_running();
	private:
		// SDL window and renderer
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

		// Camera and model
		glm::vec3 mCameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mCameraRot = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mCubeRot = glm::vec3(0.0f, 0.0f, 0.0f);
		std::unique_ptr<Model> mModel = nullptr;

	};
}
