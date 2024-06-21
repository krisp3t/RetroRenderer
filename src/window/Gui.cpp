#include "Gui.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_dx11.h>

#include "SDL_log.h"
#include "../render/DX11Globals.h"


namespace KrisRenderer {
	Gui::Gui(SDL_Window* window, const IRenderer& renderer) 
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		ImGui::StyleColorsDark();

		// TODO: check renderer type
		ImGui_ImplSDL2_InitForD3D(window);
		ImGui_ImplDX11_Init(DX11Globals::sDx11Device.Get(), DX11Globals::sDx11DeviceContext.Get());

		SDL_Log("Successfully initialized imgui");
	}

	Gui::~Gui() 
	{
		// TODO: check renderer type
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	void Gui::ProcessInput(SDL_Event& event) 
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
	}

	void Gui::BeginFrame()
	{
		ImGui_ImplDX11_NewFrame();
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
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
}
