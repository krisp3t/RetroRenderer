#include <imgui_impl_sdl2.h>
#include <SDL_log.h>

#include "Gui.h"
#include "../render/IRenderer.h"

namespace KrisRenderer {
	Gui::Gui(IRenderer &renderer) : mRenderer(&renderer)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		ImGui::StyleColorsDark();
		mRenderer->InitImgui();

		SDL_Log("Successfully initialized imgui");
	}

	Gui::~Gui() 
	{
		mRenderer->DestroyImgui();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	void Gui::ProcessInput(SDL_Event& event) 
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
	}

	void Gui::BeginFrame()
	{
		mRenderer->NewFrameImgui();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}

	void Gui::RenderScene()
	{
		bool show_demo_window = true;
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	void Gui::EndFrame() 
	{
		ImGui::Render();
		mRenderer->RenderImgui();
	}
}
