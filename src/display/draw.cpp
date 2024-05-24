
namespace MiniRenderer {
	namespace Draw {
		static void draw_pixel(int x, int y, uint32_t color, &uint32_t[] colorBuffer) {
			if (x >= 0 && x < mWinWidth && y >= 0 && y < mWinHeight) {
				colorBuffer[(mWinWidth * y) + x] = color;
			}
		}

		void draw_rect(int x, int y, int width, int height, uint32_t color) {
			for (int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {
					int current_x = x + i;
					int current_y = y + j;
					draw_pixel(current_x, current_y, color);
				}
			}
		}

		void draw_line(glm::vec2 p0, glm::vec2 p1, uint32_t color) {
			switch (mSettings->line_algo) {
			case LineAlgorithm::DDA:
				Display::draw_DDA(p0.x, p0.y, p1.x, p1.y, color);
				break;
			case LineAlgorithm::Bresenham:
				Display::draw_bresenham(p0.x, p0.y, p1.x, p1.y, color);
				break;
			case LineAlgorithm::Wu:
				Display::draw_wu(p0.x, p0.y, p1.x, p1.y, color);
				break;
			}
		}

		void draw_DDA(int x0, int y0, int x1, int y1, uint32_t color) {
			int delta_x = (x1 - x0);
			int delta_y = (y1 - y0);

			int longest_side_length = (abs(delta_x) >= abs(delta_y)) ? abs(delta_x) : abs(delta_y);

			float x_inc = delta_x / (float)longest_side_length;
			float y_inc = delta_y / (float)longest_side_length;

			float current_x = x0;
			float current_y = y0;

			for (int i = 0; i <= longest_side_length; i++) {
				draw_pixel(round(current_x), round(current_y), color);
				current_x += x_inc;
				current_y += y_inc;
			}
		}

		void draw_bresenham(int x0, int y0, int x1, int y1, uint32_t color) {
			bool steep = false;
			if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
				std::swap(x0, y0);
				std::swap(x1, y1);
				steep = true;
			}
			if (x0 > x1) {
				std::swap(x0, x1);
				std::swap(y0, y1);
			}
			int dx = x1 - x0;
			int dy = y1 - y0;
			int derror2 = std::abs(dy) * 2;
			int error2 = 0;
			int y = y0;
			for (int x = x0; x <= x1; x++) {
				if (steep) {
					draw_pixel(y, x, color);
				}
				else {
					draw_pixel(x, y, color);
				}
				error2 += derror2;
				if (error2 > dx) {
					y += (y1 > y0 ? 1 : -1);
					error2 -= dx * 2;
				}
			}
		}

		glm::vec2 project(glm::vec3 point) {
			float fov_factor = 1.0f / tan(mSettings->camera.fov / 2.0f);
			glm::vec2 projected_point = {
				(fov_factor * point.x) / point.z,
				(fov_factor * point.y) / point.z };
			return projected_point;
		}

		void fill_flat_bottom_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 mid) {
			float invslope1 = (float)(v1.x - v0.x) / (v1.y - v0.y);
			float invslope2 = (float)(mid.x - v0.x) / (mid.y - v0.y);
			float x_start = v0.x;
			float x_end = v0.x;
			int y_start = std::max(static_cast<int>(std::floor(v0.y)), 0);
			int y_end = std::min(static_cast<int>(std::ceil(v1.y)), mWinHeight - 1);
			uint32_t color = rgbaToHexArgb(mSettings->fg_color);

			// Scanline from top to flat line
			for (int y = y_start; y <= y_end; y++) {
#ifdef AVX_SUPPORTED
				__m256i colorSIMD = _mm256_set1_epi32(color);
				int ix_start = std::max(
					static_cast<int>(std::floor(x_start < x_end ? x_start : x_end)),
					0);
				int ix_end = std::min(
					static_cast<int>(std::ceil(x_end > x_start ? x_end : x_start)),
					mWinWidth - 1);
				if (ix_start >= ix_end) {
					x_start += invslope1;
					x_end += invslope2;
					continue;
				}
				int pixels_to_write = ix_end - ix_start;
				constexpr int BLOCK_SIZE = 8;
				int blockCount = pixels_to_write / BLOCK_SIZE;
				__m256i* blocks = reinterpret_cast<__m256i*>(&mColorBuffer[mWinWidth * y + ix_start]);
				// Write to blocks (8 pixels at a time)
				for (int block = 0; block < blockCount; block++) {
					_mm256_storeu_si256(blocks + block, colorSIMD);
				}
				// Set any remaining pixels individually
				for (int pixel = blockCount * BLOCK_SIZE + ix_start; pixel <= ix_end; pixel++) {
					mColorBuffer[mWinWidth * y + pixel] = color;
				}
#else
				draw_line(glm::vec2(x_start, y), glm::vec2(x_end, y), color);
#endif
				x_start += invslope1;
				x_end += invslope2;
			}
		}

		void fill_flat_top_triangle(glm::vec2 v1, glm::vec2 mid, glm::vec2 v2) {
			float invslope1 = (float)(v2.x - v1.x) / (v2.y - v1.y);
			float invslope2 = (float)(v2.x - mid.x) / (v2.y - mid.y);
			float x_start = v2.x;
			float x_end = v2.x;
			int y_start = std::min(static_cast<int>(std::ceil(v2.y)), mWinHeight - 1);
			int y_end = std::max(static_cast<int>(std::floor(v1.y)), 0);

			uint32_t color = rgbaToHexArgb(mSettings->fg_color);

			// Scanline from bottom to flat line
			for (int y = y_start; y >= y_end; y--) {
#ifdef AVX_SUPPORTED
				__m256i colorSIMD = _mm256_set1_epi32(color);

				int ix_start = std::max(
					static_cast<int>(std::floor(x_start < x_end ? x_start : x_end)),
					0);
				int ix_end = std::min(
					static_cast<int>(std::ceil(x_end > x_start ? x_end : x_start)),
					mWinWidth - 1);
				if (ix_start >= ix_end) {
					x_start -= invslope1;
					x_end -= invslope2;
					continue;
				}
				int pixels_to_write = ix_end - ix_start;
				constexpr int BLOCK_SIZE = 8;
				int blockCount = pixels_to_write / BLOCK_SIZE;
				__m256i* blocks = reinterpret_cast<__m256i*>(&mColorBuffer[mWinWidth * y + ix_start]);
				// Write to blocks (8 pixels at a time)
				for (int block = 0; block < blockCount; block++) {
					_mm256_storeu_si256(blocks + block, colorSIMD);
				}
				// Set any remaining pixels individually
				for (int pixel = blockCount * BLOCK_SIZE + ix_start; pixel <= ix_end; pixel++) {
					mColorBuffer[mWinWidth * y + pixel] = color;
				}
#else
				draw_line(glm::vec2(x_start, y), glm::vec2(x_end, y), color);
#endif
				x_start -= invslope1;
				x_end -= invslope2;
			}
		}

		void draw_triangle(std::array<glm::vec3, 3>& vertices, TriangleAlgo algo) {
			// choose draw_triangle algorithm
			switch (algo) {
			case TriangleAlgo::Wireframe:
				draw_model_wireframe(vertices);
				break;
			case TriangleAlgo::Flat:
				draw_model_flat(vertices);
				break;
			}
		}
		void draw_model_wireframe(std::array <glm::vec3, 3>& vertices) {
			// Draw triangle edges
			for (int i = 0; i < 3; i++) {
				glm::vec3 v0 = vertices[i];
				glm::vec3 v1 = vertices[(i + 1) % 3];
				glm::vec2 s0 = NDC_to_Screen(glm::vec2(v0.x, v0.y), mWinWidth, mWinHeight);
				glm::vec2 s1 = NDC_to_Screen(glm::vec2(v1.x, v1.y), mWinWidth, mWinHeight);
				draw_line(s0, s1, rgbaToHexArgb(mSettings->fg_color));
			}
		}

		void draw_model_flat(std::array <glm::vec3, 3>& vertices) {
			// Sort vertices by y-coordinate (y0 <= y1 <= y2)
			std::sort(vertices.begin(), vertices.end(), [](const glm::vec3& a, const glm::vec3& b) {
				return a.y > b.y;
				});
			// Find the triangle midpoint
			float mid_y = vertices[1].y;
			float mid_x = vertices[0].x + ((vertices[1].y - vertices[0].y) / (vertices[2].y - vertices[0].y)) * (vertices[2].x - vertices[0].x);
			glm::vec2 mid = NDC_to_Screen(glm::vec2(mid_x, mid_y), mWinWidth, mWinHeight);
			glm::vec2 v0 = NDC_to_Screen(glm::vec2(vertices[0].x, vertices[0].y), mWinWidth, mWinHeight);
			glm::vec2 v1 = NDC_to_Screen(glm::vec2(vertices[1].x, vertices[1].y), mWinWidth, mWinHeight);
			glm::vec2 v2 = NDC_to_Screen(glm::vec2(vertices[2].x, vertices[2].y), mWinWidth, mWinHeight);

			// Prevent division by zero
			if (v0.y == v1.y) {
				fill_flat_top_triangle(v0, v1, v2);
			}
			else if (v1.y == v2.y) {
				fill_flat_bottom_triangle(v0, v1, v2);
			}
			else {
				fill_flat_bottom_triangle(v0, v1, mid);
				fill_flat_top_triangle(v1, mid, v2);
			}
		}
	}
}


/*
void Display::draw_wu(int x0, int y0, int x1, int y1, uint32_t color) {
	bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
	if (steep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	float gradient = static_cast<float>(dy) / static_cast<float>(dx);

	int xend = round(x0);
	float yend = y0 + gradient * (xend - x0);
	float xgap = rfpart(x0 + 0.5);
	int xpxl1 = xend;
	int ypxl1 = ipart(yend);
	if (steep) {
		plot(ypxl1, xpxl1, rfpart(yend) * xgap, color);
		plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap, color);
	}
	else {
		plot(xpxl1, ypxl1, rfpart(yend) * xgap, color);
		plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap, color);
	}
	float intery = yend + gradient;

	xend = round(x1);
	yend = y1 + gradient * (xend - x1);
	xgap = fpart(x1 + 0.5);
	int xpxl2 = xend;
	int ypxl2 = ipart(yend);
	if (steep) {
		plot(ypxl2, xpxl2, rfpart(yend) * xgap, color);
		plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap, color);
	}
	else {
		plot(xpxl2, ypxl2, rfpart(yend) * xgap, color);
		plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap, color);
	}

	if (steep) {
		for (int x = xpxl1 + 1; x < xpxl2; x++) {
			plot(ipart(intery), x, rfpart(intery), color);
			plot(ipart(intery) + 1, x, fpart(intery), color);
			intery += gradient;
		}
	}
	else {
		for (int x = xpxl1 + 1; x < xpxl2; x++) {
			plot(x, ipart(intery), rfpart(intery), color);
			plot(x, ipart(intery) + 1, fpart(intery), color);
			intery += gradient;
		}
	}
}
void Display::plot(int x, int y, float intensity, uint32_t color) {
	uint8_t alpha = static_cast<uint8_t>(255 * intensity);
	uint32_t blendedColor = blendColors(color, alpha);
	draw_pixel(x, y, blendedColor);
}

float Display::fpart(float x) {
	return x - floor(x);
}

float Display::rfpart(float x) {
	return 1 - fpart(x);
}

int Display::ipart(float x) {
	return static_cast<int>(floor(x));
}

uint32_t Display::blendColors(uint32_t color, uint8_t alpha) {
	uint32_t r = (color >> 16) & 0xFF;
	uint32_t g = (color >> 8) & 0xFF;
	uint32_t b = color & 0xFF;
	r = (r * alpha + 255 * (255 - alpha)) / 255;
	g = (g * alpha + 255 * (255 - alpha)) / 255;
	b = (b * alpha + 255 * (255 - alpha)) / 255;
	return (r << 16) | (g << 8) | b;
}
*/