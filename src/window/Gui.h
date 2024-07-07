#pragma once
#include <functional>
#include <SDL_events.h>
#include <SDL_syswm.h>

namespace KrisRenderer
{
	class IRenderer;
	class Gui
	{
	public:
		explicit Gui(IRenderer& renderer);
		~Gui();
		void ProcessInput(SDL_Event& event);
		void BeginFrame();
		void EndFrame();
		void RenderScene();
	private:
		IRenderer* mRenderer;
	};
}
