#include "display.h"

namespace MiniRenderer {
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
		mWinWidth = display_mode.w;
		mWinHeight = display_mode.h;
		mWindow = SDL_CreateWindow(
			"MiniRenderer",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			mWinWidth,
			mWinHeight,
			SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI
		);

		if (mWindow == nullptr)
		{
			fprintf(stderr, "Error creating SDL window.\n");
			return false;
		}

		mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
		if (mRenderer == nullptr)
		{
			fprintf(stderr, "Error creating SDL renderer.\n");
			return false;
		}

		mColorBuffer = new uint32_t[mWinWidth * mWinHeight];
		// mColorBuffer = std::make_unique<uint32_t[]>(mWinWidth * mWinHeight);
		mColorBufferTexture = SDL_CreateTexture(
			mRenderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			mWinWidth,
			mWinHeight
		);
		// mGui = std::make_unique<GUI>();
		// mGui->initialize_gui(mWindow, mRenderer);
		mSettings = std::make_unique<Settings>();



		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		mIo = &ImGui::GetIO();
		mIo->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		ImGui::StyleColorsDark();

		// Setup SDLRenderer2 binding
		ImGui_ImplSDL2_InitForSDLRenderer(mWindow, mRenderer);
		ImGui_ImplSDLRenderer2_Init(mRenderer);


		mIsRunning = true;
		return true;
	}

	bool Display::is_running() {
		return mIsRunning;
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
			}
			else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
				mIsRunning = false;
			}
			break;
		}
		ImGui_ImplSDL2_ProcessEvent(&event);
	}

	void Display::update() {
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// TODO: Remove this line
		ImGui::ShowDemoWindow(nullptr);

		// Rendering settings window
		if (mSettings && mSettings->open_windows.show_renderer_window) {
			ImGui::Begin("Rendering settings");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / mIo->Framerate, mIo->Framerate);
			ImGui::End();
		}

		// Camera settings window
		if (mSettings && mSettings->open_windows.show_camera_window) {
			ImGui::Begin("Camera settings");
			ImGui::SliderFloat3("Position", &mSettings->camera.position[0], -100.0f, 100.0f);
			ImGui::SliderFloat3("Rotation", &mSettings->camera.rotation[0], -180.0f, 180.0f);
			// ImGui::Text("Camera rotation: (%.2f, %.2f, %.2f)", s->camera.rotation.x, s->camera.rotation.y, s->camera.rotation.z);
			ImGui::End();
		}
		ImGui::Render();
	}

	void Display::clear_color_buffer() {
		memset(mColorBuffer, 0xFFFFFFFF, mWinWidth * mWinHeight * sizeof(uint32_t));
	}

	void Display::render_color_buffer() {
		draw_line(0, 0, mWinWidth, mWinHeight, 0xFF00FF00);
		SDL_UpdateTexture(mColorBufferTexture, nullptr, mColorBuffer, static_cast<int>(mWinWidth * sizeof(uint32_t)));
		SDL_RenderCopy(mRenderer, mColorBufferTexture, nullptr, nullptr);
	}

	void Display::draw_pixel(int x, int y, uint32_t color) {
		if (x >= 0 && x < mWinWidth && y >= 0 && y < mWinHeight) {
			mColorBuffer[(mWinWidth * y) + x] = color;
		}
	}

	void Display::draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
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

	void Display::render() {
		Display::render_color_buffer();
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
		// mGui->render();
		SDL_RenderPresent(mRenderer);
		Display::clear_color_buffer();
	}

	void Display::destroy_window() {
		SDL_DestroyRenderer(mRenderer);
		SDL_DestroyWindow(mWindow);
		SDL_Quit();
	}

	Display::~Display()
	{
		destroy_window();
	}
}
