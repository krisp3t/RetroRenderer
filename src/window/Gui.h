#pragma once
#include <SDL_events.h>
#include <SDL_syswm.h>

namespace KrisRenderer
{
	class IRenderer;
	class Gui
	{
	public:
		Gui(SDL_Window* window, const IRenderer& renderer);
		~Gui();
		void ProcessInput(SDL_Event& event);
		void BeginFrame();
		void EndFrame();
		void RenderScene();
	};
}
