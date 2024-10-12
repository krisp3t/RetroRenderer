/***
 * Wrapper around ImGui to create a configuration panel for the renderer.
 */

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "ConfigPanel.h"

namespace RetroRenderer
{
    ConfigPanel::ConfigPanel(SDL_Window* window, SDL_Renderer* renderer)
    {
        Init(window, renderer);
    }

    ConfigPanel::~ConfigPanel()
    {
        Destroy();
    }

    bool ConfigPanel::Init(SDL_Window* window, SDL_Renderer* renderer)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        return true;
    }

    void ConfigPanel::OnDraw()
    {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        bool show = true;
        ImGui::ShowDemoWindow(&show);
        ImGui::ShowDebugLogWindow(&show);
        ImGui::ShowMetricsWindow(&show);

        // TODO: SDL_RenderSetScale?
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    }

    void ConfigPanel::Destroy()
    {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
}