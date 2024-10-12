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
#include "draw.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

namespace KrisRenderer {
	class Display
	{
	public:
		Display();
		Display(const int width, const int height);
		~Display();
		void initialize_buffers(void);
		bool initialize_window(void);
		SDL_Window* get_window(void) const;
		SDL_Renderer* get_renderer(void) const;
		void destroy_window(void);
		void process_input(void);
		void update(void);
		bool should_backside_cull(std::array<glm::vec3, 3>& vertices);
		void render_model(void);
		void render(void);
		void clear_color_buffer(void);
		void update_screen(void);
		bool is_running(void) const;
		void apply_transformations(std::array<glm::vec3, 3>& vertices);
		int mWinWidth;
		int mWinHeight;
	private:
		SDL_Window* mWindow = nullptr;
		SDL_Renderer* mRenderer = nullptr;
		std::unique_ptr<uint32_t[]> mColorBuffer = nullptr;
		SDL_Texture* mColorBufferTexture = nullptr;
		std::unique_ptr<GUI> mGui;
		std::unique_ptr<Settings> mSettings;
		ImGuiIO* mIo;
		bool mIsRunning = true;

		// Camera and model
		glm::vec3 mCameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mCameraRot = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mCubeRot = glm::vec3(0.0f, 0.0f, 0.0f);
		std::unique_ptr<Model> mModel = nullptr;

	};
}
