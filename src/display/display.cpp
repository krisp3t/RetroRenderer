#include "display.h"

namespace MiniRenderer {
	Display::Display()
	{
		mWinWidth = 1280;
		mWinHeight = 720;
	}

	Display::Display(const int width, const int height) : mWinWidth(width), mWinHeight(height) {}

	bool Display::is_running() const {
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

	void Display::clear_color_buffer() {
		memset(mColorBuffer.get(), 0x00000000, mWinWidth * mWinHeight * sizeof(uint32_t));
	}

	bool Display::initialize_window() {
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
			fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
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
		mIsRunning = true;
		return true;
	}

	SDL_Window* Display::get_window() const {
		return mWindow;
	}

	SDL_Renderer* Display::get_renderer() const {
		return mRenderer;
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
		render_model();
	}

	void Display::apply_transformations(std::array<glm::vec3, 3>& vertices) {
		for (int i = 0; i < 3; i++) {
			vertices[i].z -= mSettings->camera.near;
		}
	}

	bool Display::should_backside_cull(std::array<glm::vec3, 3>& vertices) {
		glm::vec3 normal = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
		glm::vec3 view = glm::vec3(0, 0, mSettings->camera.near);
		return glm::dot(normal, view) >= 0;
	}

	void Display::render_model() {
		if (mModel == nullptr) {
			return;
		}
		for (const auto& face : mModel->faces()) {
			std::array<glm::vec3, 3> vertices = {
				mModel->vert(face.positionIndices[0]),
				mModel->vert(face.positionIndices[1]),
				mModel->vert(face.positionIndices[2])
			};
			apply_transformations(vertices);
			if ((mSettings->backface_culling) && (!should_backside_cull(vertices))) {
				continue;
			}
			// Draw::draw_triangle(vertices, TriangleAlgo::Wireframe);
		};
	}

	void Display::update_screen() {
		int pitch = mWinWidth * sizeof(uint32_t);
		void* pixels;
		if (SDL_LockTexture(mColorBufferTexture, nullptr, &pixels, &pitch) == 0) {
			memcpy(pixels, mColorBuffer.get(), pitch * mWinHeight);
			SDL_UnlockTexture(mColorBufferTexture);
		}
		else {
			SDL_Log("Failed to lock color buffer texture: %s", SDL_GetError());
			return;
		}
	}

	void Display::render() {
		// Copy color buffer to texture, which is then rendered to the screen
		SDL_RenderClear(mRenderer);
		//SDL_UpdateTexture(mColorBufferTexture, nullptr, mColorBuffer.get(), mWinWidth * sizeof(uint32_t));
		update_screen();
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
