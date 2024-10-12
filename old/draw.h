#pragma once
#ifdef __AVX__
#include <immintrin.h>
#define AVX_SUPPORTED true
#else
#define AVX_SUPPORTED false
#endif

#include "display.h"


namespace KrisRenderer {
	namespace Draw {
		void draw_pixel(int x, int y, uint32_t color, uint32_t colorBuffer[]);
		void draw_triangle(std::array<glm::vec3, 3>& vertices, TriangleAlgo algo);
		void draw_rect(int x, int y, int width, int height, uint32_t color);
		void draw_line(glm::vec2 p0, glm::vec2 p1, uint32_t color);
		void draw_DDA(int x0, int y0, int x1, int y1, uint32_t color);
		void draw_bresenham(int x0, int y0, int x1, int y1, uint32_t color);
		void draw_wu(int x0, int y0, int x1, int y1, uint32_t color);
		glm::vec2 project(glm::vec3 point);
		void fill_flat_bottom_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 mid);
		void fill_flat_top_triangle(glm::vec2 v1, glm::vec2 mid, glm::vec2 v2);
		void draw_triangle(std::array<glm::vec3, 3>& vertices, TriangleAlgo algo);
		void draw_model_wireframe(std::array <glm::vec3, 3>& vertices);
		void draw_model_flat(std::array <glm::vec3, 3>& vertices);
	}
}