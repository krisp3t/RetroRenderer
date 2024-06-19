#pragma once
#include <SDL.h>
#include <utility>
#include <cassert>
#include <memory>
#include "../render/IRenderer.h"
#include "../render/SWRenderer.h"
#include "../defines.h"


#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

namespace KrisRenderer
{
	class Window
	{
	public:
		Window();
		Window(int width, int height);
		~Window();
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) noexcept = default;
		Window& operator=(Window&&) noexcept = default;

		bool InitializeWindow();
		std::pair<int, int> GetSize() const;

		SDL_Window* GetWindow() const;
		bool IsRunning() const;
		void HandleEvents();
		void Update();
		void Render();
		// std::unique_ptr<IRenderer> GetRenderer() const;
	private:
		SDL_Window* mWindow = nullptr;
		std::unique_ptr<IRenderer> mRenderer;
		int mWinWidth;
		int mWinHeight;
		bool mIsRunning = true;
	};
}