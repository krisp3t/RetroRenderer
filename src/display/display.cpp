#include "display.h"


namespace MiniRenderer {
	bool Display::is_running() {
		return mIsRunning;
	}

	void Display::initialize_buffers() {
		// We write to the color buffer in 32-bit ARGB format
		// On every frame, we copy the color buffer to a texture
		// which is then copied to GPU and rendered to the screen
		mColorBuffer = new uint32_t[mWinWidth * mWinHeight];
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

		initialize_buffers();
		mSettings = std::make_unique<Settings>(mWinWidth, mWinHeight);
		mModel = std::make_unique<Model>("head.obj");
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

	std::string filePathName;
	std::string filePath;

	glm::vec2 Display::project(glm::vec3 point)
	{
		float fov_factor = 1.0f / tan(mSettings->camera.fov / 2.0f);
		glm::vec2 projected_point = {
			(fov_factor * point.x) / point.z,
			(fov_factor * point.y) / point.z };
		return projected_point;
	}

	void Display::update() {
		bool modelUpdated = mGui->update(*mSettings);
		if (modelUpdated) {
			mModel = std::make_unique<Model>(mSettings->filename);
		}
		draw_model();
	}

	void Display::draw_model() {
		if (mModel == nullptr || (mSettings->triangle_algo != TriangleAlgo::Wireframe)) {
			return;
		}
		for (const auto& face : mModel->faces()) {
			const std::array<int, 3>& vertex_indices = face.positionIndices;
			for (int i = 0; i < 3; i++) {
				glm::vec3 v0 = mModel->vert(vertex_indices[i]);
				glm::vec3 v1 = mModel->vert(vertex_indices[(i + 1) % 3]);
				if (mSettings->triangle_algo == TriangleAlgo::Wireframe) {
					int x0 = (v0.x + 1.) * mWinWidth / 2.;
					int y0 = (v0.y + 1.) * mWinHeight / 2.;
					int x1 = (v1.x + 1.) * mWinWidth / 2.;
					int y1 = (v1.y + 1.) * mWinHeight / 2.;
					draw_line(x0, y0, x1, y1, 0xFFFF0000);
				}
			}
		}

	}



	void Display::clear_color_buffer() {
		memset(mColorBuffer, 0x00000000, mWinWidth * mWinHeight * sizeof(uint32_t));
		/*
		__m256i colorSIMD = _mm256_set1_epi32(0xFF00FF00);
		int blockCount = static_cast<int>(mWinWidth * mWinHeight / 8);
		__m256i* blocks = (__m256i*) mColorBuffer;

		//SIMD as much as possible
		for (int block = 0; block < blockCount; ++block) {
			blocks[block] = colorSIMD;
		}

		//set any remaining pixels individually
		for (int pixel = blockCount * 8; pixel < mWinWidth * mWinHeight; ++pixel) {
			mColorBuffer[pixel] = 0xFFFF0000;
		}
		*/
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

	void Display::draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
		switch (mSettings->line_algo) {
		case LineAlgorithm::DDA:
			Display::draw_DDA(x0, y0, x1, y1, color);
			break;
		case LineAlgorithm::Bresenham:
			Display::draw_bresenham(x0, y0, x1, y1, color);
			break;
		case LineAlgorithm::Wu:
			Display::draw_wu(x0, y0, x1, y1, color);
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
		SDL_UpdateTexture(mColorBufferTexture, nullptr, mColorBuffer, mWinWidth * sizeof(uint32_t));
		SDL_RenderCopy(mRenderer, mColorBufferTexture, nullptr, nullptr);
		mGui->render();
		SDL_RenderPresent(mRenderer);
		Display::clear_color_buffer();
	}

	void Display::destroy_window() {
		delete mColorBuffer,
		SDL_DestroyTexture(mColorBufferTexture);
		SDL_DestroyRenderer(mRenderer);
		SDL_DestroyWindow(mWindow);
		SDL_Quit();
	}

	Display::~Display() {
		destroy_window();
	}
}
