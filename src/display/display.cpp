#include "display.h"


namespace MiniRenderer {
	bool Display::is_running() {
		return mIsRunning;
	}

	void Display::initialize_buffers() {
		// We write to the color buffer in 32-bit ARGB format
		// On every frame, we copy the color buffer to a texture
		// which is then copied to GPU and rendered to the screen

		mColorBuffer = std::make_unique<uint32_t[]>(mWinWidth * mWinHeight);
		mColorBufferTexture = SDL_CreateTexture(
			mRenderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			mWinWidth,
			mWinHeight
		);
	}

	bool Display::initialize_window() {
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
			printf("Error: %s\n", SDL_GetError());
			return false;
		}

#ifdef SDL_HINT_IME_SHOW_UI
		SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

		SDL_DisplayMode display_mode;
		SDL_GetCurrentDisplayMode(0, &display_mode);
		mWinWidth = 1280;
		mWinHeight = 720;
		mWindow = SDL_CreateWindow(
			"MiniRenderer",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			mWinWidth,
			mWinHeight,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
		);

		if (mWindow == nullptr)
		{
			fprintf(stderr, "Error creating SDL window.\n");
			return false;
		}

		mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);
		if (mRenderer == nullptr)
		{
			fprintf(stderr, "Error creating SDL renderer.\n");
			return false;
		}
		SDL_Log("AVX support: %s", AVX_SUPPORTED ? "true" : "false");

		initialize_buffers();
		mSettings = std::make_unique<Settings>(mWinWidth, mWinHeight);
		mSettings->filename = "head.obj";
		mModel = std::make_unique<Model>(mSettings->filename);
		mGui = std::make_unique<GUI>();
		mGui->initialize_gui(mWindow, mRenderer);
		mIsRunning = true;
		return true;
	}

	void Display::process_input() {
		SDL_Event event;
		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			mIsRunning = false;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				mIsRunning = false;
			break;
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				mWinWidth = event.window.data1;
				mWinHeight = event.window.data2;
				SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window.data2);
				initialize_buffers();
			}
			else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
				mIsRunning = false;
			}
			break;
		}
		mGui->process_input(event);
	}

	glm::vec2 Display::project(glm::vec3 point)
	{
		float fov_factor = 1.0f / tan(mSettings->camera.fov / 2.0f);
		glm::vec2 projected_point = {
			(fov_factor * point.x) / point.z,
			(fov_factor * point.y) / point.z };
		return projected_point;
	}

	void Display::update() {
		GUIState state = mGui->update(*mSettings, *mModel);
		switch (state) {
		case GUIState::None:
			break;
		case GUIState::LoadModel:
			mModel = std::make_unique<Model>(mSettings->filename);
			break;
		case GUIState::SaveScreenshot:
			break;
		}
		draw_model();
	}

	void Display::fill_flat_bottom_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 mid) {
		float invslope1 = (float)(v1.x - v0.x) / (v1.y - v0.y);
		float invslope2 = (float)(mid.x - v0.x) / (mid.y - v0.y);
		float x_start = v0.x;
		float x_end = v0.x;
		uint32_t color = rgbaToHexArgb(mSettings->fg_color);

		for (int y = static_cast<int>(v0.y); y <= static_cast<int>(v1.y); y++) {
			x_start += invslope1;
			x_end += invslope2;
			if (x_start >= x_end) {
				break;
			}
#ifdef AVX_SUPPORTED
			__m256i colorSIMD = _mm256_set1_epi32(color);
			int ix_start = static_cast<int>(x_start);
			int ix_end = static_cast<int>(x_end);
			int pixels_to_write = ix_end - ix_start;
			int blockCount = pixels_to_write / 8;
			__m256i* blocks = reinterpret_cast<__m256i*>(&mColorBuffer[mWinWidth * y + ix_start]);
			// Write to blocks (8 pixels at a time)
			for (int block = 0; block < blockCount; block++) {
				_mm256_storeu_si256(blocks + block, colorSIMD);
			}

			// Set any remaining pixels individually
			for (int pixel = blockCount * 8 + ix_start; pixel < ix_end; pixel++) {
				mColorBuffer[mWinWidth * y + pixel] = color;
			}
#else
			draw_line(glm::vec2(x_start, y), glm::vec2(x_end, y), color);
#endif
		}
	}

	void Display::fill_flat_top_triangle(glm::vec2 v1, glm::vec2 mid, glm::vec2 v2) {
		float invslope1 = (float)(mid.x - v1.x) / (mid.y - v1.y);
		float invslope2 = (float)(v2.x - v1.x) / (v2.y - v1.y);
		float x_start = v1.x;
		float x_end = v1.x;

		for (int y = v1.y; y <= v2.y; y++) {
			x_start -= invslope1;
			x_end -= invslope2;
			// draw_line(p0, p1, rgbaToHex(mSettings->fg_color));
			//draw_line(glm::vec2(x_start, y), glm::vec2(x_end, y), rgbaToHex(mSettings->fg_color));
		}
	}

	void Display::draw_model_flat(std::array <glm::vec3, 3>& vertices) {
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

		fill_flat_bottom_triangle(v0, v1, mid);
		// fill_flat_top_triangle(v1, mid, v2);
	}

	void Display::draw_model_wireframe(std::array <glm::vec3, 3>& vertices) {
		// Draw triangle edges
		for (int i = 0; i < 3; i++) {
			glm::vec3 v0 = vertices[i];
			glm::vec3 v1 = vertices[(i + 1) % 3];
			glm::vec2 s0 = NDC_to_Screen(glm::vec2(v0.x, v0.y), mWinWidth, mWinHeight);
			glm::vec2 s1 = NDC_to_Screen(glm::vec2(v1.x, v1.y), mWinWidth, mWinHeight);
			draw_line(s0, s1, rgbaToHexArgb(mSettings->fg_color));
		}
	}

	void Display::apply_transformations(std::array<glm::vec3, 3>& vertices) {
		for (int i = 0; i < 3; i++) {
			vertices[i].z -= mSettings->camera.near;
		}
	}

	bool Display::backside_cull(std::array<glm::vec3, 3>& vertices) {
		glm::vec3 normal = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
		glm::vec3 view = glm::vec3(0, 0, 1);
		return glm::dot(normal, view) < 0;
	}

	void Display::draw_model() {
		if (mModel == nullptr) {
			return;
		}
		std::function<void(std::array<glm::vec3, 3>&)> draw_triangle = [](std::array<glm::vec3, 3>& vertices) {};
		switch (mSettings->triangle_algo) {
		case TriangleAlgo::Wireframe:
			draw_triangle = [this](std::array<glm::vec3, 3>& vertices) {
				draw_model_wireframe(vertices);
				};
			break;
		case TriangleAlgo::Flat:
			draw_triangle = [this](std::array<glm::vec3, 3>& vertices) {
				draw_model_flat(vertices);
				};
			break;
		}
		for (const auto& face : mModel->faces()) {
			std::array<glm::vec3, 3> vertices = {
				mModel->vert(face.positionIndices[0]),
				mModel->vert(face.positionIndices[1]),
				mModel->vert(face.positionIndices[2])
			};
			apply_transformations(vertices);
			if ((mSettings->backface_culling) && (backside_cull(vertices))) {
				continue;
			}
			draw_triangle(vertices);
		};
	}



	void Display::clear_color_buffer() {
		memset(mColorBuffer.get(), 0x00000000, mWinWidth * mWinHeight * sizeof(uint32_t));
	}

	void Display::draw_pixel(int x, int y, uint32_t color) {
		if (x >= 0 && x < mWinWidth && y >= 0 && y < mWinHeight) {
			mColorBuffer[(mWinWidth * y) + x] = color;
		}
	}
	void Display::draw_DDA(int x0, int y0, int x1, int y1, uint32_t color) {
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

	void Display::draw_bresenham(int x0, int y0, int x1, int y1, uint32_t color) {
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

	void Display::draw_line(glm::vec2 p0, glm::vec2 p1, uint32_t color) {
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



	void Display::draw_rect(int x, int y, int width, int height, uint32_t color) {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int current_x = x + i;
				int current_y = y + j;
				draw_pixel(current_x, current_y, color);
			}
		}
	}

	void Display::render() {
		// Copy color buffer to texture, which is then rendered to the screen
		SDL_RenderClear(mRenderer);
		SDL_UpdateTexture(mColorBufferTexture, nullptr, mColorBuffer.get(), mWinWidth * sizeof(uint32_t));
		SDL_RenderCopy(mRenderer, mColorBufferTexture, nullptr, nullptr);
		mGui->render();
		SDL_RenderPresent(mRenderer);
		Display::clear_color_buffer();
	}

	void Display::destroy_window() {
		SDL_DestroyTexture(mColorBufferTexture);
		SDL_DestroyRenderer(mRenderer);
		SDL_DestroyWindow(mWindow);
		SDL_Quit();
	}

	Display::~Display() {
		destroy_window();
	}
}
